//  Copyright (c) 2022-2025 Fredrik Mellbin
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BSSHARED_H
#define BSSHARED_H

#include <memory>
#include <cstdio>
#include <string>
#include <stdexcept>
#include <functional>
#include <filesystem>
#include <vector>
#include <cstdint>

constexpr size_t HashSize = 8;

class BestSourceException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class BestSourceHWDecoderException : public BestSourceException {
    using BestSourceException::BestSourceException;
};

// FIXME, technically undefined behavior since FILE isn't a user defined type
namespace std {
    template<>
    struct default_delete<FILE> {
        void operator()(FILE *Ptr) {
            fclose(Ptr);
        }
    };
}

typedef std::unique_ptr<FILE> file_ptr_t;

typedef std::function<bool(int Track, int64_t Current, int64_t Total)> ProgressFunction;

enum BestCacheMode {
    bcmDisable = 0,
    bcmAutoSubTree,
    bcmAlwaysWriteSubTree,
    bcmAutoAbsolutePath,
    bcmAlwaysAbsolutePath,
};

struct AVRational;

struct BSRational {
    int Num;
    int Den;
    BSRational() = default;
    BSRational(const AVRational &r);
    double ToDouble() const;
};

std::filesystem::path CreateProbablyUTF8Path(const char *Filename);

int SetFFmpegLogLevel(int Level);

void SetBSDebugOutput(bool DebugOutput);
void BSDebugPrint(const std::string_view Message, int64_t RequestedN = -1, int64_t CurrentN = -1);

bool ShouldWriteIndex(int CacheMode, size_t Frames);
bool IsAbsolutePathCacheMode(int CacheMode);

file_ptr_t OpenNormalFile(const std::filesystem::path &Filename, bool Write);
file_ptr_t OpenCacheFile(bool AbsolutePath, const std::filesystem::path &CacheBasePath, const std::filesystem::path &Source, int Track, bool Write);
void WriteByte(file_ptr_t &F, uint8_t Value);
void WriteInt(file_ptr_t &F, int Value);
void WriteInt64(file_ptr_t &F, int64_t Value);
void WriteDouble(file_ptr_t &F, double Value);
void WriteString(file_ptr_t &F, const std::string &Value);
void WriteBSHeader(file_ptr_t &F, bool Video);
uint8_t ReadByte(file_ptr_t &F);
int ReadInt(file_ptr_t &F);
int64_t ReadInt64(file_ptr_t &F);
double ReadDouble(file_ptr_t &F);
std::string ReadString(file_ptr_t &F);
bool ReadCompareInt(file_ptr_t &F, int Value);
bool ReadCompareInt64(file_ptr_t &F, int64_t Value);
bool ReadCompareDouble(file_ptr_t &F, double Value);
bool ReadBSHeader(file_ptr_t &F, bool Video);
bool PlausibleRecordCount(file_ptr_t &F, int64_t Count, size_t MinRecordBytes);
bool CloseWrittenFile(file_ptr_t &F);

/* Maps a selected format set's frame numbers to positions in the full track index. Choosing one
   set out of several drops the other frames, so the selected numbering the caller uses no longer
   lines up with the index; this records the correspondence once, when the set is selected, so
   every lookup is O(1) and every caller agrees on it instead of rescanning the whole index (and
   disagreeing about which numbering it produced). Returned empty when there is nothing to remap
   -- no selection, or a single format set -- which callers read as the identity mapping. Match
   decides which index frames belong to the selected set. */
template<typename FrameVec, typename MatchFn>
std::vector<int64_t> BuildSelectedFrameMapping(const FrameVec &Frames, bool Active, MatchFn Match) {
    std::vector<int64_t> Map;
    if (!Active)
        return Map;
    Map.reserve(Frames.size());
    for (int64_t i = 0; i < static_cast<int64_t>(Frames.size()); i++)
        if (Match(Frames[i]))
            Map.push_back(i);
    return Map;
}

#endif