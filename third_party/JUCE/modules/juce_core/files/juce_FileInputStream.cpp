/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/
#include <string.h>;
#include <emscripten.h>;

namespace juce
{

int64 juce_fileSetPosition (void* handle, int64 pos);

EM_JS(void, prepareForFileRead, (char* str), {
    let filePath = "";
    let ptr = str;
    while (HEAPU8[ptr] > 0) {
        filePath += String.fromCharCode(HEAPU8[ptr]);
        ptr++;
    }
    filePath = filePath.trim();
    const splits = filePath.split("/");
    const entName = "./" + splits[splits.length - 2] + "/" + splits[splits.length - 1];
    const entry = FILE_OVERRIDES[entName];
    if (!entry) {
        console.log("No entry for ", entName);
        return;
    }
    if (entry instanceof Uint8Array) {
        console.log("Entry already patched: ", entName);
        return;
    }
    if (entry instanceof Blob) {
        console.log("[JS] Shimming read request for : ", filePath);

        const SAB = new SharedArrayBuffer(1 + entry.size); //first byte is for signalling
        const view = new Uint8Array(SAB);
        globalThis.fileReadWorker.postMessage({
            SAB: SAB,
            lockPtr: 0,
            file: entry
        });
        while (Atomics.compareExchange(view, 0, 1, 2) !== 1) {};
        FILE_OVERRIDES[entName] = view.subarray(1, view.length);

        let targetWritePath = entName;
        let foundSymlink = false;
        Object.entries(globalThis.ASSET_MOUNTPOINTS).forEach(ent => {
            if (targetWritePath.includes(ent[0])) {
                foundSymlink = true;
                targetWritePath = targetWritePath.replace(ent[0], ent[1]);
            }
        });
        if (!foundSymlink) {
            targetWritePath = filePath;
        }
        
        Vial.FS.writeFile(targetWritePath, FILE_OVERRIDES[entName]);
        console.log("Shimmed read was successful!");

        FILE_DEALLOC_QUEUE.add({
            targetWritePath,
            readTime: Date.now(),
            entName,
            entry,
            gcBuffer: FILE_OVERRIDES[entName]
        });
        // todo: add a queue of files to delete (push buffers to WeakSet along with paths and read time, and deallocate after 30s?) !!! Should reduce memory consumption
    }
});

//==============================================================================
FileInputStream::FileInputStream (const File& f)  : file (f)
{
    //std::cout << "[C] Found read request for : " << f.getFullPathName().toStdString() << std::endl;
    std::string fname = f.getFullPathName().toStdString();
    char* buffer = (char*)malloc(fname.size() + 1);
    strcpy(buffer, fname.c_str());
    prepareForFileRead(buffer);
    free(buffer);
    openHandle();
}

int64 FileInputStream::getTotalLength()
{
    // You should always check that a stream opened successfully before using it!
    jassert (openedOk());

    return file.getSize();
}

int FileInputStream::read (void* buffer, int bytesToRead)
{
    // You should always check that a stream opened successfully before using it!
    jassert (openedOk());

    // The buffer should never be null, and a negative size is probably a
    // sign that something is broken!
    jassert (buffer != nullptr && bytesToRead >= 0);

    auto num = readInternal (buffer, (size_t) bytesToRead);
    currentPosition += (int64) num;

    return (int) num;
}

bool FileInputStream::isExhausted()
{
    return currentPosition >= getTotalLength();
}

int64 FileInputStream::getPosition()
{
    return currentPosition;
}

bool FileInputStream::setPosition (int64 pos)
{
    // You should always check that a stream opened successfully before using it!
    jassert (openedOk());

    if (pos != currentPosition)
        currentPosition = juce_fileSetPosition (fileHandle, pos);

    return currentPosition == pos;
}


//==============================================================================
//==============================================================================
#if JUCE_UNIT_TESTS

struct FileInputStreamTests   : public UnitTest
{
    FileInputStreamTests()
        : UnitTest ("FileInputStream", UnitTestCategories::streams)
    {}

    void runTest() override
    {
        beginTest ("Open stream non-existent file");
        {
            auto tempFile = File::createTempFile (".txt");
            expect (! tempFile.exists());

            FileInputStream stream (tempFile);
            expect (stream.failedToOpen());
        }

        beginTest ("Open stream existing file");
        {
            auto tempFile = File::createTempFile (".txt");
            tempFile.create();
            expect (tempFile.exists());

            FileInputStream stream (tempFile);
            expect (stream.openedOk());
        }

        const MemoryBlock data ("abcdefghijklmnopqrstuvwxyz", 26);
        File f (File::createTempFile (".txt"));
        f.appendData (data.getData(), data.getSize());
        FileInputStream stream (f);

        beginTest ("Read");
        {
            expectEquals (stream.getPosition(), (int64) 0);
            expectEquals (stream.getTotalLength(), (int64) data.getSize());
            expectEquals (stream.getNumBytesRemaining(), stream.getTotalLength());
            expect (! stream.isExhausted());

            size_t numBytesRead = 0;
            MemoryBlock readBuffer (data.getSize());

            while (numBytesRead < data.getSize())
            {
                numBytesRead += (size_t) stream.read (&readBuffer[numBytesRead], 3);

                expectEquals (stream.getPosition(), (int64) numBytesRead);
                expectEquals (stream.getNumBytesRemaining(), (int64) (data.getSize() - numBytesRead));
                expect (stream.isExhausted() == (numBytesRead == data.getSize()));
            }

            expectEquals (stream.getPosition(), (int64) data.getSize());
            expectEquals (stream.getNumBytesRemaining(), (int64) 0);
            expect (stream.isExhausted());

            expect (readBuffer == data);
        }

        beginTest ("Skip");
        {
            stream.setPosition (0);
            expectEquals (stream.getPosition(), (int64) 0);
            expectEquals (stream.getTotalLength(), (int64) data.getSize());
            expectEquals (stream.getNumBytesRemaining(), stream.getTotalLength());
            expect (! stream.isExhausted());

            size_t numBytesRead = 0;
            const int numBytesToSkip = 5;

            while (numBytesRead < data.getSize())
            {
                stream.skipNextBytes (numBytesToSkip);
                numBytesRead += numBytesToSkip;
                numBytesRead = std::min (numBytesRead, data.getSize());

                expectEquals (stream.getPosition(), (int64) numBytesRead);
                expectEquals (stream.getNumBytesRemaining(), (int64) (data.getSize() - numBytesRead));
                expect (stream.isExhausted() == (numBytesRead == data.getSize()));
            }

            expectEquals (stream.getPosition(), (int64) data.getSize());
            expectEquals (stream.getNumBytesRemaining(), (int64) 0);
            expect (stream.isExhausted());

            f.deleteFile();
        }
    }
};

static FileInputStreamTests fileInputStreamTests;

#endif

} // namespace juce
