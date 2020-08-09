/* Copyright (c) 2020 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

#include <libjamsat/proof/DRATCertificate.h>

#include <libjamsat/proof/BinaryDRATEncoder.h>

#include <cstdio>
#include <vector>

namespace jamsat {
namespace {

class FileDRATCertificate : public DRATCertificate {
public:
    explicit FileDRATCertificate(std::string const& path)
      : m_file{nullptr}, m_buffer{}, m_fileBuffer{} {
        m_file = fopen(path.data(), "w");
        if (m_file == nullptr) {
            throw FileIOError();
        }

        m_buffer.resize(1);
        m_fileBuffer.resize(1024 * 1024);
        std::setvbuf(m_file, m_fileBuffer.data(), _IOFBF, m_fileBuffer.size());
    }

    void addRATClause(gsl::span<CNFLit const> clause, size_t pivotIdx) override {
        if (pivotIdx == 0) {
            writeLiterals(clause, true);
        } else {
            writeLiteralsPivotFirst(clause, true, pivotIdx);
        }
    }

    void addATClause(gsl::span<CNFLit const> clause) override { writeLiterals(clause, true); }

    void deleteClause(gsl::span<CNFLit const> clause) override { writeLiterals(clause, false); }

    void flush() override {
        if (fflush(m_file) != 0) {
            throw FileIOError();
        }
    }

    virtual ~FileDRATCertificate() {
        if (m_file != nullptr) {
            fclose(m_file);
            m_file = nullptr;
        }
    }

private:
    void writeLiterals(gsl::span<CNFLit const> clause, bool added) {
        ensureBufferLargeEnough(clause.size());
        m_buffer[0] = added ? 0x61 : 0x64;
        std::size_t encodingLen = EncodeBinaryDRAT(clause, gsl::span{m_buffer}.subspan(1));
        ++encodingLen; // accounting for m_buffer[0]
        m_buffer[encodingLen] = 0;

        std::size_t itemsWritten = fwrite(m_buffer.data(), encodingLen + 1, 1, m_file);
        if (itemsWritten != 1) {
            throw FileIOError();
        }
    }

    void writeLiteralsPivotFirst(gsl::span<CNFLit const> clause, bool added, std::size_t pivotIdx) {
        ensureBufferLargeEnough(clause.size());
        m_buffer[0] = added ? 0x61 : 0x64;

        std::size_t encodingLen =
            EncodeBinaryDRAT(clause.subspan(pivotIdx, 1), gsl::span{m_buffer}.subspan(1));
        ++encodingLen; // accounting for m_buffer[0]

        encodingLen +=
            EncodeBinaryDRAT(clause.subspan(0, pivotIdx), gsl::span{m_buffer}.subspan(encodingLen));
        encodingLen += EncodeBinaryDRAT(clause.subspan(pivotIdx + 1),
                                        gsl::span{m_buffer}.subspan(encodingLen));
        m_buffer[encodingLen] = 0;

        std::size_t itemsWritten = fwrite(m_buffer.data(), encodingLen + 1, 1, m_file);
        if (itemsWritten != 1) {
            throw FileIOError();
        }
    }

    void ensureBufferLargeEnough(std::size_t numLits) {
        std::size_t requiredBufferSize = 5 * numLits + 2;
        if (m_buffer.size() < requiredBufferSize) {
            m_buffer.resize(requiredBufferSize);
        }
    }

    FILE* m_file;
    std::vector<unsigned char> m_buffer;
    std::vector<char> m_fileBuffer;
};
}

std::unique_ptr<DRATCertificate> createFileDRATCertificate(std::string const& path) {
    return std::make_unique<FileDRATCertificate>(path);
}
}
