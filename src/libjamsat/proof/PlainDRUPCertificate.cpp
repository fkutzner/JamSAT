/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

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

#include <libjamsat/proof/DRUPCertificate.h>

namespace jamsat {

class PlainDRUPCertificate : public DRUPCertificate {
public:
    explicit PlainDRUPCertificate(std::ostream &outputStream);
    virtual ~PlainDRUPCertificate();

    PlainDRUPCertificate &operator=(const PlainDRUPCertificate &other) = delete;
    PlainDRUPCertificate &operator=(PlainDRUPCertificate &&other) = delete;
    PlainDRUPCertificate(const PlainDRUPCertificate &other) = delete;
    PlainDRUPCertificate(PlainDRUPCertificate &&other) = delete;

    void closeProof() override;

protected:
    void beginDeletedClause() override;
    void addLiteral(CNFLit lit) override;
    void endClause() override;

private:
    std::ostream &m_outputStream;
};

std::unique_ptr<DRUPCertificate> createPlainDRUPCertificate(std::ostream &outputStream) {
    return std::make_unique<PlainDRUPCertificate>(outputStream);
}

PlainDRUPCertificate::PlainDRUPCertificate(std::ostream &outputStream)
  : DRUPCertificate(), m_outputStream(outputStream) {}

PlainDRUPCertificate::~PlainDRUPCertificate() {}

void PlainDRUPCertificate::closeProof() {
    m_outputStream << "0\n";
    m_outputStream.flush();
}

void PlainDRUPCertificate::beginDeletedClause() {
    m_outputStream << "d ";
}

void PlainDRUPCertificate::addLiteral(CNFLit lit) {
    m_outputStream << lit << " ";
}

void PlainDRUPCertificate::endClause() {
    m_outputStream << "0\n";
}
}
