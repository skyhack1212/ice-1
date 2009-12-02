// **********************************************************************
//
// Copyright (c) 2003-2009 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <IceUtil/DisableWarnings.h>
#include <Gen.h>
#include <Slice/Util.h>
#include <Slice/CPlusPlusUtil.h>
#include <IceUtil/Functional.h>
#include <IceUtil/Iterator.h>
#include <Slice/Checksum.h>
#include <Slice/FileTracker.h>

#include <limits>

#include <sys/stat.h>
#include <string.h>

using namespace std;
using namespace Slice;
using namespace IceUtil;
using namespace IceUtilInternal;

static string
getDeprecateSymbol(const ContainedPtr& p1, const ContainedPtr& p2)
{
    string deprecateMetadata, deprecateSymbol;
    if(p1->findMetaData("deprecate", deprecateMetadata) ||
       (p2 != 0 && p2->findMetaData("deprecate", deprecateMetadata)))
    {
        deprecateSymbol = "ICE_DEPRECATED_API ";
    }
    return deprecateSymbol;
}

Slice::Gen::Gen(const string& base, const string& headerExtension, const string& sourceExtension,
                const vector<string>& extraHeaders, const string& include,
                const vector<string>& includePaths, const string& dllExport, const string& dir,
                bool imp, bool checksum, bool stream, bool ice) :
    _base(base),
    _headerExtension(headerExtension),
    _implHeaderExtension(headerExtension),
    _sourceExtension(sourceExtension),
    _extraHeaders(extraHeaders),
    _include(include),
    _includePaths(includePaths),
    _dllExport(dllExport),
    _dir(dir),
    _impl(imp),
    _checksum(checksum),
    _stream(stream),
    _ice(ice)
{
    for(vector<string>::iterator p = _includePaths.begin(); p != _includePaths.end(); ++p)
    {
        *p = fullPath(*p);
    }

    string::size_type pos = _base.find_last_of("/\\");
    if(pos != string::npos)
    {
        _base.erase(0, pos + 1);
    }
}

Slice::Gen::~Gen()
{
    H << "\n\n#endif\n";
    C << '\n';

    if(_impl)
    {
        implH << "\n\n#endif\n";
        implC << '\n';
    }
}

void
Slice::Gen::generate(const UnitPtr& p)
{
    string file = p->topLevelFile();

    //
    // Give precedence to header-ext global metadata.
    //
    string headerExtension = getHeaderExt(file, p);
    if(!headerExtension.empty())
    {
        _headerExtension = headerExtension;
    }

    if(_impl)
    {
        string fileImplH = _base + "I." + _implHeaderExtension;
        string fileImplC = _base + "I." + _sourceExtension;
        if(!_dir.empty())
        {
            fileImplH = _dir + '/' + fileImplH;
            fileImplC = _dir + '/' + fileImplC;
        }

        struct stat st;
        if(stat(fileImplH.c_str(), &st) == 0)
        {
            ostringstream os;
            os << fileImplH << "' already exists - will not overwrite";
            throw FileException(__FILE__, __LINE__, os.str());
        }
        if(stat(fileImplC.c_str(), &st) == 0)
        {
            ostringstream os;
            os << fileImplC << "' already exists - will not overwrite";
            throw FileException(__FILE__, __LINE__, os.str());
        }

        implH.open(fileImplH.c_str());
        if(!implH)
        {
            ostringstream os;
            os << "cannot open `" << fileImplH << "': " << strerror(errno);
            throw FileException(__FILE__, __LINE__, os.str());
        }
        FileTracker::instance()->addFile(fileImplH);

        implC.open(fileImplC.c_str());
        if(!implC)
        {
            ostringstream os;
            os << "cannot open `" << fileImplC << "': " << strerror(errno);
            throw FileException(__FILE__, __LINE__, os.str());
        }
        FileTracker::instance()->addFile(fileImplC);

        string s = fileImplH;
        if(_include.size())
        {
            s = _include + '/' + s;
        }
        transform(s.begin(), s.end(), s.begin(), ToIfdef());
        implH << "#ifndef __" << s << "__";
        implH << "\n#define __" << s << "__";
        implH << '\n';
    }

    string fileH = _base + "." + _headerExtension;
    string fileC = _base + "." + _sourceExtension;
    if(!_dir.empty())
    {
        fileH = _dir + '/' + fileH;
        fileC = _dir + '/' + fileC;
    }

    H.open(fileH.c_str());
    if(!H)
    {
        ostringstream os;
        os << "cannot open `" << fileH << "': " << strerror(errno);
        throw FileException(__FILE__, __LINE__, os.str());
    }
    FileTracker::instance()->addFile(fileH);

    C.open(fileC.c_str());
    if(!C)
    {
        ostringstream os;
        os << "cannot open `" << fileC << "': " << strerror(errno);
        throw FileException(__FILE__, __LINE__, os.str());
    }
    FileTracker::instance()->addFile(fileC);

    printHeader(H);
    printGeneratedHeader(H, _base + ".ice");
    printHeader(C);
    printGeneratedHeader(C, _base + ".ice");


    string s = fileH;
    if(_include.size())
    {
        s = _include + '/' + s;
    }
    transform(s.begin(), s.end(), s.begin(), ToIfdef());
    H << "\n#ifndef __" << s << "__";
    H << "\n#define __" << s << "__";
    H << '\n';

    validateMetaData(p);

    writeExtraHeaders(C);

    if(_dllExport.size())
    {
        C << "\n#ifndef " << _dllExport << "_EXPORTS";
        C << "\n#   define " << _dllExport << "_EXPORTS";
        C << "\n#endif";
    }

    C << "\n#include <";
    if(_include.size())
    {
        C << _include << '/';
    }
    C << _base << "." << _headerExtension << ">";


    H << "\n#include <Ice/LocalObjectF.h>";
    H << "\n#include <Ice/ProxyF.h>";
    H << "\n#include <Ice/ObjectF.h>";
    H << "\n#include <Ice/Exception.h>";
    H << "\n#include <Ice/LocalObject.h>";

    if(p->usesProxies())
    {
        H << "\n#include <Ice/Proxy.h>";
    }

    if(p->hasNonLocalClassDefs())
    {
        H << "\n#include <Ice/Object.h>";
        H << "\n#include <Ice/Outgoing.h>";
	H << "\n#include <Ice/OutgoingAsync.h>";
        H << "\n#include <Ice/Incoming.h>";
        if(p->hasContentsWithMetaData("amd"))
        {
            H << "\n#include <Ice/IncomingAsync.h>";
        }
        H << "\n#include <Ice/Direct.h>";

        C << "\n#include <Ice/LocalException.h>";
        C << "\n#include <Ice/ObjectFactory.h>";
    }
    else if(p->hasNonLocalClassDecls())
    {

        H << "\n#include <Ice/Object.h>";
    }

    if(p->hasNonLocalDataOnlyClasses() || p->hasNonLocalExceptions())
    {
        H << "\n#include <Ice/FactoryTableInit.h>";
    }
    H << "\n#include <IceUtil/ScopedArray.h>";

    if(p->usesNonLocals())
    {
        C << "\n#include <Ice/BasicStream.h>";

        if(!p->hasNonLocalClassDefs() && !p->hasNonLocalClassDecls())
        {
            C << "\n#include <Ice/Object.h>";
        }
    }

    if(_stream || p->hasNonLocalClassDefs() || p->hasNonLocalExceptions())
    {
        if(!p->hasNonLocalClassDefs())
        {
            C << "\n#include <Ice/LocalException.h>";
        }

        if(_stream)
        {
            H << "\n#include <Ice/Stream.h>";
        }
    }

    if(_checksum)
    {
        C << "\n#include <Ice/SliceChecksums.h>";
    }

    C << "\n#include <IceUtil/Iterator.h>";

    StringList includes = p->includeFiles();

    for(StringList::const_iterator q = includes.begin(); q != includes.end(); ++q)
    {
        string extension = getHeaderExt((*q), p);
        if(extension.empty())
        {
            extension = _headerExtension;
        }
        H << "\n#include <" << changeInclude(*q, _includePaths) << "." << extension << ">";
    }

    H << "\n#include <Ice/UndefSysMacros.h>";

    if(_ice)
    {
        C << "\n#include <IceUtil/DisableWarnings.h>";
    }

    //
    // Emit #include statements for any cpp:include metadata directives
    // in the top-level Slice file.
    //
    {
        DefinitionContextPtr dc = p->findDefinitionContext(file);
        assert(dc);
        StringList globalMetaData = dc->getMetaData();
        for(StringList::const_iterator q = globalMetaData.begin(); q != globalMetaData.end(); ++q)
        {
            string s = *q;
            static const string includePrefix = "cpp:include:";
            if(s.find(includePrefix) == 0 && s.size() > includePrefix.size())
            {
                H << nl << "#include <" << s.substr(includePrefix.size()) << ">";
            }
        }
    }

    printVersionCheck(H);
    printVersionCheck(C);

    printDllExportStuff(H, _dllExport);
    if(_dllExport.size())
    {
        _dllExport += " ";
    }

    ProxyDeclVisitor proxyDeclVisitor(H, C, _dllExport);
    p->visit(&proxyDeclVisitor, false);

    ObjectDeclVisitor objectDeclVisitor(H, C, _dllExport);
    p->visit(&objectDeclVisitor, false);

    IceInternalVisitor iceInternalVisitor(H, C, _dllExport);
    p->visit(&iceInternalVisitor, false);

    HandleVisitor handleVisitor(H, C, _dllExport, _stream);
    p->visit(&handleVisitor, false);

    TypesVisitor typesVisitor(H, C, _dllExport, _stream);
    p->visit(&typesVisitor, false);

    AsyncVisitor asyncVisitor(H, C, _dllExport);
    p->visit(&asyncVisitor, false);

    AsyncImplVisitor asyncImplVisitor(H, C, _dllExport);
    p->visit(&asyncImplVisitor, false);

    //
    // The templates are emitted before the proxy definition
    // so the derivation hierarchy is known to the proxy:
    // the proxy relies on knowing the hierarchy to make the begin_
    // methods type-safe.
    //
    AsyncCallbackVisitor asyncCallbackVisitor(H, C, _dllExport);
    p->visit(&asyncCallbackVisitor, false);

    ProxyVisitor proxyVisitor(H, C, _dllExport);
    p->visit(&proxyVisitor, false);

    DelegateVisitor delegateVisitor(H, C, _dllExport);
    p->visit(&delegateVisitor, false);

    DelegateMVisitor delegateMVisitor(H, C, _dllExport);
    p->visit(&delegateMVisitor, false);

    DelegateDVisitor delegateDVisitor(H, C, _dllExport);
    p->visit(&delegateDVisitor, false);

    ObjectVisitor objectVisitor(H, C, _dllExport, _stream);
    p->visit(&objectVisitor, false);

    if(_stream)
    {
        StreamVisitor streamVistor(H, C);
        p->visit(&streamVistor, false);
    }

    //
    // We need to delay generating the template after the proxy
    // definition, because __completed calls the begin_ method in the
    // proxy.
    //
    AsyncCallbackTemplateVisitor asyncCallbackTemplateVisitor(H, C, _dllExport);
    p->visit(&asyncCallbackTemplateVisitor, false);

    if(_impl)
    {
        implH << "\n#include <";
        if(_include.size())
        {
            implH << _include << '/';
        }
        implH << _base << "." << _headerExtension << ">";

        writeExtraHeaders(implC);

        implC << "\n#include <";
        if(_include.size())
        {
            implC << _include << '/';
        }
        implC << _base << "I." << _implHeaderExtension << ">";

        ImplVisitor implVisitor(implH, implC, _dllExport);
        p->visit(&implVisitor, false);
    }

    if(_checksum)
    {
        ChecksumMap map = createChecksums(p);
        if(!map.empty())
        {
            C << sp << nl << "static const char* __sliceChecksums[] =";
            C << sb;
            for(ChecksumMap::const_iterator q = map.begin(); q != map.end(); ++q)
            {
                C << nl << "\"" << q->first << "\", \"";
                ostringstream str;
                str.flags(ios_base::hex);
                str.fill('0');
                for(vector<unsigned char>::const_iterator r = q->second.begin(); r != q->second.end(); ++r)
                {
                    str << (int)(*r);
                }
                C << str.str() << "\",";
            }
            C << nl << "0";
            C << eb << ';';
            C << nl << "static IceInternal::SliceChecksumInit __sliceChecksumInit(__sliceChecksums);";
        }
    }
}

void
Slice::Gen::closeOutput()
{
    H.close();
    C.close();
    implH.close();
    implC.close();
}

void
Slice::Gen::writeExtraHeaders(IceUtilInternal::Output& out)
{
    for(vector<string>::const_iterator i = _extraHeaders.begin(); i != _extraHeaders.end(); ++i)
    {
        string hdr = *i;
        string guard;
        string::size_type pos = hdr.rfind(',');
        if(pos != string::npos)
        {
            hdr = i->substr(0, pos);
            guard = i->substr(pos + 1);
        }
        if(!guard.empty())
        {
            out << "\n#ifndef " << guard;
            out << "\n#define " << guard;
        }
        out << "\n#include <";
        if(!_include.empty())
        {
            out << _include << '/';
        }
        out << hdr << '>';
        if(!guard.empty())
        {
            out << "\n#endif";
        }
    }
}

Slice::Gen::TypesVisitor::TypesVisitor(Output& h, Output& c, const string& dllExport, bool stream) :
    H(h), C(c), _dllExport(dllExport), _stream(stream), _doneStaticSymbol(false), _useWstring(false)
{
}

bool
Slice::Gen::TypesVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasOtherConstructedOrExceptions())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    H << sp << nl << "namespace " << fixKwd(p->name()) << nl << '{';

    return true;
}

void
Slice::Gen::TypesVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::TypesVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    return false;
}

bool
Slice::Gen::TypesVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    ExceptionPtr base = p->base();
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList allDataMembers = p->allDataMembers();
    DataMemberList::const_iterator q;

    vector<string> params;
    vector<string> allTypes;
    vector<string> allParamDecls;
    vector<string> baseParams;
    vector<string>::const_iterator pi;

    for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
        params.push_back(fixKwd((*q)->name()));
    }

    for(q = allDataMembers.begin(); q != allDataMembers.end(); ++q)
    {
        string typeName = inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
        allTypes.push_back(typeName);
        allParamDecls.push_back(typeName + " __ice_" + fixKwd((*q)->name()));
    }

    if(base)
    {
        DataMemberList baseDataMembers = base->allDataMembers();
        for(q = baseDataMembers.begin(); q != baseDataMembers.end(); ++q)
        {
            baseParams.push_back("__ice_" + fixKwd((*q)->name()));
        }
    }

    H << sp << nl << "class " << _dllExport << name << " : ";
    H.useCurrentPosAsIndent();
    H << "public ";
    if(!base)
    {
        H << (p->isLocal() ? "::Ice::LocalException" : "::Ice::UserException");
    }
    else
    {
        H << fixKwd(base->scoped());
    }
    H.restoreIndent();
    H << sb;

    H.dec();
    H << nl << "public:";
    H.inc();

    H << sp << nl << name << spar;
    if(p->isLocal())
    {
        H << "const char*" << "int";
    }
    H << epar;
    if(!p->isLocal())
    {
        H << " {}";
    }
    else
    {
        H << ';';
    }
    if(!allTypes.empty())
    {
        H << nl;
        if(!p->isLocal() && allTypes.size() == 1)
        {
            H << "explicit ";
        }
        H << name << spar;
        if(p->isLocal())
        {
            H << "const char*" << "int";
        }
        H << allTypes << epar << ';';
    }
    H << nl << "virtual ~" << name << "() throw();";
    H << sp;

    if(p->isLocal())
    {
        C << sp << nl << scoped.substr(2) << "::" << name << spar << "const char* __file" << "int __line" << epar
          << " :";
        C.inc();
        emitUpcall(base, "(__file, __line)", true);
        C.dec();
        C << sb;
        C << eb;
    }

    if(!allTypes.empty())
    {
        C << sp << nl;
        C << scoped.substr(2) << "::" << name << spar;
        if(p->isLocal())
        {
            C << "const char* __file" << "int __line";
        }
        C << allParamDecls << epar;
        if(p->isLocal() || !baseParams.empty() || !params.empty())
        {
            C << " :";
            C.inc();
            string upcall;
            if(!allParamDecls.empty())
            {
                upcall = "(";
                if(p->isLocal())
                {
                    upcall += "__file, __line";
                }
                for(pi = baseParams.begin(); pi != baseParams.end(); ++pi)
                {
                    if(p->isLocal() || pi != baseParams.begin())
                    {
                        upcall += ", ";
                    }
                    upcall += *pi;
                }
                upcall += ")";
            }
            if(!params.empty())
            {
                upcall += ",";
            }
            emitUpcall(base, upcall, p->isLocal());
        }
        for(pi = params.begin(); pi != params.end(); ++pi)
        {
            if(pi != params.begin())
            {
                C << ",";
            }
            C << nl << *pi << "(__ice_" << *pi << ')';
        }
        if(p->isLocal() || !baseParams.empty() || !params.empty())
        {
            C.dec();
        }
        C << sb;
        C << eb;
    }

    C << sp << nl;
    C << scoped.substr(2) << "::~" << name << "() throw()";
    C << sb;
    C << eb;

    H << nl << "virtual ::std::string ice_name() const;";

    string flatName = p->flattenedScope() + p->name() + "_name";

    C << sp << nl << "static const char* " << flatName << " = \"" << p->scoped().substr(2) << "\";";
    C << sp << nl << "::std::string" << nl << scoped.substr(2) << "::ice_name() const";
    C << sb;
    C << nl << "return " << flatName << ';';
    C << eb;

    if(p->isLocal())
    {
        H << nl << "virtual void ice_print(::std::ostream&) const;";
    }

    H << nl << "virtual ::Ice::Exception* ice_clone() const;";
    C << sp << nl << "::Ice::Exception*" << nl << scoped.substr(2) << "::ice_clone() const";
    C << sb;
    C << nl << "return new " << name << "(*this);";
    C << eb;

    H << nl << "virtual void ice_throw() const;";
    C << sp << nl << "void" << nl << scoped.substr(2) << "::ice_throw() const";
    C << sb;
    C << nl << "throw *this;";
    C << eb;

    if(!p->isLocal())
    {
        H << sp << nl << "static const ::IceInternal::UserExceptionFactoryPtr& ice_factory();";
    }
    if(!dataMembers.empty())
    {
        H << sp;
    }
    return true;
}

void
Slice::Gen::TypesVisitor::visitExceptionEnd(const ExceptionPtr& p)
{
    string name = fixKwd(p->name());
    string scope = fixKwd(p->scope());
    string scoped = fixKwd(p->scoped());
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList::const_iterator q;

    string factoryName;

    if(!p->isLocal())
    {
        ExceptionPtr base = p->base();

        H << sp << nl << "virtual void __write(::IceInternal::BasicStream*) const;";
        H << nl << "virtual void __read(::IceInternal::BasicStream*, bool);";
        
        H.zeroIndent();
        H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
        H.restoreIndent();
        H << nl << "//Stream api is not supported in VC++ 6";
        H.zeroIndent();
        H << nl << "#else";
        H.restoreIndent();
        H << nl << "virtual void __write(const ::Ice::OutputStreamPtr&) const;";
        H << nl << "virtual void __read(const ::Ice::InputStreamPtr&, bool);";
        H.zeroIndent();
        H << nl << "#endif";
        H.restoreIndent();
        
        C << sp << nl << "void" << nl << scoped.substr(2) << "::__write(::IceInternal::BasicStream* __os) const";
        C << sb;
        C << nl << "__os->write(::std::string(\"" << p->scoped() << "\"), false);";
        C << nl << "__os->startWriteSlice();";
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            writeMarshalUnmarshalCode(C, (*q)->type(), fixKwd((*q)->name()), true, "", true, (*q)->getMetaData());
        }
        C << nl << "__os->endWriteSlice();";
        if(base)
        {
            emitUpcall(base, "::__write(__os);");
        }
        C << eb;

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__read(::IceInternal::BasicStream* __is, bool __rid)";
        C << sb;
        C << nl << "if(__rid)";
        C << sb;
        C << nl << "::std::string myId;";
        C << nl << "__is->read(myId, false);";
        C << eb;
        C << nl << "__is->startReadSlice();";
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            writeMarshalUnmarshalCode(C, (*q)->type(), fixKwd((*q)->name()), false, "", true, (*q)->getMetaData());
        }
        C << nl << "__is->endReadSlice();";
        if(base)
        {
            emitUpcall(base, "::__read(__is, true);");
        }
        C << eb;

        if(_stream)
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp << nl << "void" << nl << scoped.substr(2)
              << "::__write(const ::Ice::OutputStreamPtr& __outS) const";
            C << sb;
            C << nl << "__outS->writeString(::std::string(\"" << p->scoped() << "\"));";
            C << nl << "__outS->startSlice();";
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                writeStreamMarshalUnmarshalCode(C, (*q)->type(), (*q)->name(), true, "", (*q)->getMetaData(),
                                                _useWstring);
            }
            C << nl << "__outS->endSlice();";
            if(base)
            {
                emitUpcall(base, "::__write(__outS);");
            }
            C << eb;

            C << sp << nl << "void" << nl << scoped.substr(2)
              << "::__read(const ::Ice::InputStreamPtr& __inS, bool __rid)";
            C << sb;
            C << nl << "if(__rid)";
            C << sb;
            C << nl << "__inS->readString();";
            C << eb;
            C << nl << "__inS->startSlice();";
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                writeStreamMarshalUnmarshalCode(C, (*q)->type(), (*q)->name(), false, "", (*q)->getMetaData(), 
                                                _useWstring);
            }
            C << nl << "__inS->endSlice();";
            if(base)
            {
                emitUpcall(base, "::__read(__inS, true);");
            }
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
        else
        {
            //
            // Emit placeholder functions to catch errors.
            //
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp << nl << "void" << nl << scoped.substr(2) << "::__write(const ::Ice::OutputStreamPtr&) const";
            C << sb;
            C << nl << "Ice::MarshalException ex(__FILE__, __LINE__);";
            C << nl << "ex.reason = \"exception " << scoped.substr(2) << " was not generated with stream support\";";
            C << nl << "throw ex;";
            C << eb;

            C << sp << nl << "void" << nl << scoped.substr(2) << "::__read(const ::Ice::InputStreamPtr&, bool)";
            C << sb;
            C << nl << "Ice::MarshalException ex(__FILE__, __LINE__);";
            C << nl << "ex.reason = \"exception " << scoped .substr(2)<< " was not generated with stream support\";";
            C << nl << "throw ex;";
            C << eb;
            
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }

        if(p->usesClasses())
        {
            if(!base || (base && !base->usesClasses()))
            {
                H << nl << "virtual bool __usesClasses() const;";

                C << sp << nl << "bool";
                C << nl << scoped.substr(2) << "::__usesClasses() const";
                C << sb;
                C << nl << "return true;";
                C << eb;
            }
        }

        factoryName = "__F" + p->flattenedScope() + p->name();

        C << sp << nl << "struct " << factoryName << " : public ::IceInternal::UserExceptionFactory";
        C << sb;
        C << sp << nl << "virtual void";
        C << nl << "createAndThrow()";
        C << sb;
        C << nl << "throw " << scoped << "();";
        C << eb;
        C << eb << ';';

        C << sp << nl << "static ::IceInternal::UserExceptionFactoryPtr " << factoryName
          << "__Ptr = new " << factoryName << ';';

        C << sp << nl << "const ::IceInternal::UserExceptionFactoryPtr&";
        C << nl << scoped.substr(2) << "::ice_factory()";
        C << sb;
        C << nl << "return " << factoryName << "__Ptr;";
        C << eb;

        C << sp << nl << "class " << factoryName << "__Init";
        C << sb;
        C.dec();
        C << nl << "public:";
        C.inc();
        C << sp << nl << factoryName << "__Init()";
        C << sb;
        C << nl << "::IceInternal::factoryTable->addExceptionFactory(\"" << p->scoped() << "\", " << scoped
          << "::ice_factory());";
        C << eb;
        C << sp << nl << "~" << factoryName << "__Init()";
        C << sb;
        C << nl << "::IceInternal::factoryTable->removeExceptionFactory(\"" << p->scoped() << "\");";
        C << eb;
        C << eb << ';';
        C << sp << nl << "static " << factoryName << "__Init "<< factoryName << "__i;";
        C << sp << nl << "#ifdef __APPLE__";

        string initfuncname = "__F" + p->flattenedScope() + p->name() + "__initializer";
        C << nl << "extern \"C\" { void " << initfuncname << "() {} }";
        C << nl << "#endif";
    }
    H << eb << ';';

    if(!p->isLocal())
    {
        //
        // We need an instance here to trigger initialization if the implementation is in a shared libarry.
        // But we do this only once per source file, because a single instance is sufficient to initialize
        // all of the globals in a shared library.
        //
        if(!_doneStaticSymbol)
        {
            _doneStaticSymbol = true;
            H << sp << nl << "static " << name << " __" << p->name() << "_init;";
        }
    }

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::TypesVisitor::visitStructStart(const StructPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    if(findMetaData(p->getMetaData()) == "class")
    {
        H << sp << nl << "class " << _dllExport << name << " : public IceUtil::Shared";
        H << sb;
        H.dec();
        H << nl << "public:";
        H.inc();
        H << nl;
        H << nl << name << "() {}";

        DataMemberList dataMembers = p->dataMembers();
        if(!dataMembers.empty())
        {
            DataMemberList::const_iterator q;
            vector<string> paramDecls;
            vector<string> types;
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                string typeName = inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
                types.push_back(typeName);
                paramDecls.push_back(typeName + " __ice_" + (*q)->name());
            }

            H << nl;
            if(paramDecls.size() == 1)
            {
                H << "explicit ";
            }
            H << name << spar << types << epar << ';';

            C << sp << nl << fixKwd(p->scoped()).substr(2) << "::"
              << fixKwd(p->name()) << spar << paramDecls << epar << " :";
            C.inc();

            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                if(q != dataMembers.begin())
                {
                    C << ',' << nl;
                }
                string memberName = fixKwd((*q)->name());
                C << memberName << '(' << "__ice_" << (*q)->name() << ')';
            }

            C.dec();
            C << sb;
            C << eb;
        }
    }
    else
    {
        H << sp << nl << "struct " << name;
        H << sb;
    }

    return true;
}

void
Slice::Gen::TypesVisitor::visitStructEnd(const StructPtr& p)
{
    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    string scope = fixKwd(p->scope());

    DataMemberList dataMembers = p->dataMembers();
    DataMemberList::const_iterator q;

    vector<string> params;
    vector<string>::const_iterator pi;

    for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
        params.push_back(fixKwd((*q)->name()));
    }

    string dllExport;
    if(findMetaData(p->getMetaData()) != "class")
    {
        dllExport = _dllExport;
    }

    H << sp << nl << "bool operator==(const " << name << "& __rhs) const";
    H << sb;
    H << nl << "if(this == &__rhs)";
    H << sb;
    H << nl << "return true;";
    H << eb;
    for(pi = params.begin(); pi != params.end(); ++pi)
    {
        H << nl << "if(" << *pi << " != __rhs." << *pi << ')';
        H << sb;
        H << nl << "return false;";
        H << eb;
    }
    H << nl << "return true;";
    H << eb;
    H << sp << nl << "bool operator<(const " << name << "& __rhs) const";
    H << sb;
    H << nl << "if(this == &__rhs)";
    H << sb;
    H << nl << "return false;";
    H << eb;
    for(pi = params.begin(); pi != params.end(); ++pi)
    {
        H << nl << "if(" << *pi << " < __rhs." << *pi << ')';
        H << sb;
        H << nl << "return true;";
        H << eb;
        H << nl << "else if(__rhs." << *pi << " < " << *pi << ')';
        H << sb;
        H << nl << "return false;";
        H << eb;
    }
    H << nl << "return false;";
    H << eb;

    H << sp << nl << "bool operator!=(const " << name << "& __rhs) const";
    H << sb;
    H << nl << "return !operator==(__rhs);";
    H << eb;
    H << nl << "bool operator<=(const " << name << "& __rhs) const";
    H << sb;
    H << nl << "return operator<(__rhs) || operator==(__rhs);";
    H << eb;
    H << nl << "bool operator>(const " << name << "& __rhs) const";
    H << sb;
    H << nl << "return !operator<(__rhs) && !operator==(__rhs);";
    H << eb;
    H << nl << "bool operator>=(const " << name << "& __rhs) const";
    H << sb;
    H << nl << "return !operator<(__rhs);";
    H << eb;

    if(!p->isLocal())
    {
        //
        // None of these member functions is virtual!
        //
        H << sp << nl << dllExport << "void __write(::IceInternal::BasicStream*) const;";
        H << nl << dllExport << "void __read(::IceInternal::BasicStream*);";

        if(_stream)
        {
            H.zeroIndent();
            H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            H.restoreIndent();
            H << nl << "//Stream api is not supported in VC++ 6";
            H.zeroIndent();
            H << nl << "#else";
            H.restoreIndent();
            H << sp << nl << dllExport << "void ice_write(const ::Ice::OutputStreamPtr&) const;";
            H << nl << dllExport << "void ice_read(const ::Ice::InputStreamPtr&);";
            H.zeroIndent();
            H << nl << "#endif";
            H.restoreIndent();
        }

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__write(::IceInternal::BasicStream* __os) const";
        C << sb;
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            writeMarshalUnmarshalCode(C, (*q)->type(), fixKwd((*q)->name()), true, "", true, (*q)->getMetaData());
        }
        C << eb;

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__read(::IceInternal::BasicStream* __is)";
        C << sb;
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            writeMarshalUnmarshalCode(C, (*q)->type(), fixKwd((*q)->name()), false, "", true, (*q)->getMetaData());
        }
        C << eb;

        if(_stream)
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp << nl << "void" << nl << scoped.substr(2)
              << "::ice_write(const ::Ice::OutputStreamPtr& __outS) const";
            C << sb;
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                writeStreamMarshalUnmarshalCode(C, (*q)->type(), (*q)->name(), true, "", (*q)->getMetaData(), 
                                                _useWstring);
            }
            C << eb;

            C << sp << nl << "void" << nl << scoped.substr(2) << "::ice_read(const ::Ice::InputStreamPtr& __inS)";
            C << sb;
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                writeStreamMarshalUnmarshalCode(C, (*q)->type(), (*q)->name(), false, "", (*q)->getMetaData(), 
                                                _useWstring);
            }
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
    }

    H << eb << ';';
    if(findMetaData(p->getMetaData()) == "class")
    {
        H << sp << nl << "typedef ::IceUtil::Handle< " << scoped << "> " << p->name() + "Ptr;";

        if(!p->isLocal() && _stream)
        {
            H.zeroIndent();
            H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            H.restoreIndent();
            H << nl << "//Stream api is not supported in VC++ 6";
            H.zeroIndent();
            H << nl << "#else";
            H.restoreIndent();
            H << sp << nl << "void ice_write" << p->name() << "(const ::Ice::OutputStreamPtr&, const "
              << p->name() << "Ptr&);";
            H << nl << "void ice_read" << p->name() << "(const ::Ice::InputStreamPtr&, " << p->name()
              << "Ptr&);";
            H.zeroIndent();
            H << nl << "#endif";
            H.restoreIndent();
              
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp << nl << "void" << nl << scope.substr(2) << "ice_write" << p->name()
              << "(const ::Ice::OutputStreamPtr& __outS, const " << fixKwd(p->scoped() + "Ptr") << "& __v)";
            C << sb;
            C << nl << "__v->ice_write(__outS);";
            C << eb;

            C << sp << nl << "void" << nl << scope.substr(2) << "ice_read" << p->name()
              << "(const ::Ice::InputStreamPtr& __inS, " << fixKwd(p->scoped() + "Ptr") << "& __v)";
            C << sb;
            C << nl << "__v->ice_read(__inS);";
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
    }
    else
    {
        if(!p->isLocal() && _stream)
        {
            H.zeroIndent();
            H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            H.restoreIndent();
            H << nl << "//Stream api is not supported in VC++ 6";
            H.zeroIndent();
            H << nl << "#else";
            H.restoreIndent();
            H << sp << nl << dllExport << "void ice_write" << p->name() << "(const ::Ice::OutputStreamPtr&, const "
              << name << "&);";
            H << nl << dllExport << "void ice_read" << p->name() << "(const ::Ice::InputStreamPtr&, " << name << "&);";
            H.zeroIndent();
            H << nl << "#endif";
            H.restoreIndent();
            
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp << nl << "void" << nl << scope.substr(2) << "ice_write" << p->name()
              << "(const ::Ice::OutputStreamPtr& __outS, const " << scoped << "& __v)";
            C << sb;
            C << nl << "__v.ice_write(__outS);";
            C << eb;

            C << sp << nl << "void" << nl << scope.substr(2) << "ice_read" << p->name()
              << "(const ::Ice::InputStreamPtr& __inS, " << scoped << "& __v)";
            C << sb;
            C << nl << "__v.ice_read(__inS);";
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
    }

    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::TypesVisitor::visitDataMember(const DataMemberPtr& p)
{
    string name = fixKwd(p->name());
    TypePtr type = p->type();
    if(p->container() != 0 && (StructPtr::dynamicCast(p->container()) || ExceptionPtr::dynamicCast(p->container())) &&
       SequencePtr::dynamicCast(type))
    {
        SequencePtr s = SequencePtr::dynamicCast(type);
        BuiltinPtr builtin = BuiltinPtr::dynamicCast(s->type());
        if(builtin && builtin->kind() == Builtin::KindByte)
        {
            StringList metaData = s->getMetaData();
            bool protobuf;
            findMetaData(s, metaData, protobuf);
            if(protobuf)
            {
                emitWarning(p->file(), p->line(), string("protobuf cannot be used as a ") + 
                                                  (StructPtr::dynamicCast(p->container()) ? "struct" : "exception") +
                                                  " member in C++");
            }
        }
    }

    string s = typeToString(p->type(), p->getMetaData(), _useWstring);
    H << nl << s << ' ' << name << ';';
}

void
Slice::Gen::TypesVisitor::visitSequence(const SequencePtr& p)
{
    string name = fixKwd(p->name());
    TypePtr type = p->type();
    string s = typeToString(type, p->typeMetaData(), _useWstring);
    StringList metaData = p->getMetaData();

    bool protobuf;
    string seqType = findMetaData(p, metaData, protobuf);
    H << sp;
    if(!protobuf)
    {
        if(!seqType.empty())
        {
            H << nl << "typedef " << seqType << ' ' << name << ';';
        }
        else
        {
            H << nl << "typedef ::std::vector<" << (s[0] == ':' ? " " : "") << s << "> " << name << ';';
        }
    }

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if(!p->isLocal())
    {
        string scoped = fixKwd(p->scoped());
        string scope = fixKwd(p->scope());

        if(protobuf || !seqType.empty())
        {
            string typeName = name;
            string scopedName = scoped;
            if(protobuf && !seqType.empty())
            {
                typeName = seqType;
                scopedName = seqType;
            }
            H << nl << _dllExport << "void __write" << name << "(::IceInternal::BasicStream*, const "
              << typeName << "&);";
            H << nl << _dllExport << "void __read" << name << "(::IceInternal::BasicStream*, "
              << typeName << "&);";

            if(_stream)
            {
                H.zeroIndent();
                H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
                H.restoreIndent();
                H << nl << "//Stream api is not supported in VC++ 6";
                H.zeroIndent();
                H << nl << "#else";
                H.restoreIndent();
                H << nl << _dllExport << "ICE_DEPRECATED_API void ice_write" << p->name() << "(const ::Ice::OutputStreamPtr&, const "
                  << typeName << "&);";
                H << nl << _dllExport << "ICE_DEPRECATED_API void ice_read" << p->name() << "(const ::Ice::InputStreamPtr&, " << typeName
                  << "&);";
                H.zeroIndent();
                H << nl << "#endif";
                H.restoreIndent();
            }

            C << sp << nl << "void" << nl << scope.substr(2) << "__write" << name <<
                "(::IceInternal::BasicStream* __os, const " << scopedName << "& v)";
            C << sb;
            if(protobuf)
            {
                C << nl << "std::vector< ::Ice::Byte> data(v.ByteSize());";
                C << nl << "if(!v.IsInitialized())";
                C << sb;
                C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"type not fully initialized: \" + v.InitializationErrorString());";
                C << eb;
                C << nl << "if(!v.SerializeToArray(&data[0], data.size()))";
                C << sb;
                C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"SerializeToArray failed\");";
                C << eb;
                C << nl << "__os->write(&data[0], &data[0] + data.size());";
            }
            else
            {
                C << nl << "::Ice::Int size = static_cast< ::Ice::Int>(v.size());";
                C << nl << "__os->writeSize(size);";
                C << nl << "for(" << name << "::const_iterator p = v.begin(); p != v.end(); ++p)";
                C << sb;
                writeMarshalUnmarshalCode(C, type, "(*p)", true);
                C << eb;
            }
            C << eb;

            C << sp << nl << "void" << nl << scope.substr(2) << "__read" << name
              << "(::IceInternal::BasicStream* __is, " << scopedName << "& v)";
            C << sb;
            if(protobuf)
            {
                C << nl << "::std::pair<const ::Ice::Byte*, const ::Ice::Byte*> data;";
                C << nl << "__is->read(data);";
                C << nl << "if(!v.ParseFromArray(data.first, data.second - data.first))";
                C << sb;
                C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"ParseFromArray failed\");";
                C << eb;
            }
            else
            {
                C << nl << "::Ice::Int sz;";
                C << nl << "__is->readSize(sz);";
                C << nl << name << "(sz).swap(v);";
                if(type->isVariableLength())
                {
                    // Protect against bogus sequence sizes.
                    C << nl << "__is->startSeq(sz, " << type->minWireSize() << ");";
                }
                else
                {
                    C << nl << "__is->checkFixedSeq(sz, " << type->minWireSize() << ");";
                }
                C << nl << "for(" << name << "::iterator p = v.begin(); p != v.end(); ++p)";
                C << sb;
                writeMarshalUnmarshalCode(C, type, "(*p)", false);

                //
                // After unmarshaling each element, check that there are still enough bytes left in the stream
                // to unmarshal the remainder of the sequence, and decrement the count of elements
                // yet to be unmarshaled for sequences with variable-length element type (that is, for sequences
                // of classes, structs, dictionaries, sequences, strings, or proxies). This allows us to
                // abort unmarshaling for bogus sequence sizes at the earliest possible moment.
                // (For fixed-length sequences, we don't need to do this because the prediction of how many
                // bytes will be taken up by the sequence is accurate.)
                //
                if(type->isVariableLength())
                {
                    if(!SequencePtr::dynamicCast(type))
                    {
                        //
                        // No need to check for directly nested sequences because, at the start of each
                        // sequence, we check anyway.
                        //
                        C << nl << "__is->checkSeq();";
                    }
                    C << nl << "__is->endElement();";
                }
                C << eb;
                if(type->isVariableLength())
                {
                    C << nl << "__is->endSeq(sz);";
                }
            }
            C << eb;

            if(_stream)
            {
                C.zeroIndent();
                C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
                C.restoreIndent();
                C << nl << "//Stream api is not supported in VC++ 6";
                C.zeroIndent();
                C << nl << "#else";
                C.restoreIndent();
                C << sp << nl << "void" << nl << scope.substr(2) << "ice_write" << p->name()
                  << "(const ::Ice::OutputStreamPtr& __outS, const " << scopedName << "& v)";
                C << sb;
                if(protobuf)
                {
                    C << nl << "std::vector< ::Ice::Byte> data(v.ByteSize());";
                    C << nl << "if(!v.IsInitialized())";
                    C << sb;
                    C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"type not fully initialized: \" + v.InitializationErrorString());";
                    C << eb;
                    C << nl << "v.SerializeToArray(&data[0], data.size());";

                    C << nl << "__outS->writeByteSeq(data);";
                }
                else
                {
                    C << nl << "__outS->writeSize(::Ice::Int(v.size()));";
                    C << nl << scopedName << "::const_iterator p;";
                    C << nl << "for(p = v.begin(); p != v.end(); ++p)";
                    C << sb;
                    writeStreamMarshalUnmarshalCode(C, type, "(*p)", true, "", StringList(), _useWstring);
                    C << eb;
                }
                C << eb;

                C << sp << nl << "void" << nl << scope.substr(2) << "ice_read" << p->name()
                  << "(const ::Ice::InputStreamPtr& __inS, " << scopedName << "& v)";
                C << sb;
                if(protobuf)
                {
                    C << nl << "std::pair<const ::Ice::Byte*, const ::Ice::Byte*> data;";
                    C << nl << "__inS->readByteSeq(data);";
                    C << nl << "if(!v.ParseFromArray(data.first, data.second - data.first))";
                    C << sb;
                    C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"ParseFromArray failed\");";
                    C << eb;
                }
                else
                {
                    C << nl << "::Ice::Int sz = __inS->readSize();";
                    C << nl << scopedName << "(sz).swap(v);";
                    C << nl << scopedName << "::iterator p;";
                    C << nl << "for(p = v.begin(); p != v.end(); ++p)";
                    C << sb;
                    writeStreamMarshalUnmarshalCode(C, type, "(*p)", false, "", StringList(), _useWstring);
                    C << eb;
                }
                C << eb;
                C.zeroIndent();
                C << nl << "#endif";
                C.restoreIndent();
            }
        }
        else if(!builtin || builtin->kind() == Builtin::KindObject || builtin->kind() == Builtin::KindObjectProxy)
        {
            H << nl << _dllExport << "void __write" << name << "(::IceInternal::BasicStream*, const " << s
              << "*, const " << s << "*);";
            H << nl << _dllExport << "void __read" << name << "(::IceInternal::BasicStream*, " << name << "&);";

            if(_stream)
            {
                H.zeroIndent();
                H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
                H.restoreIndent();
                H << nl << "//Stream api is not supported in VC++ 6";
                H.zeroIndent();
                H << nl << "#else";
                H.restoreIndent();
                H << nl << _dllExport << "ICE_DEPRECATED_API void ice_write" << p->name() << "(const ::Ice::OutputStreamPtr&, const "
                  << name << "&);";
                H << nl << _dllExport << "ICE_DEPRECATED_API void ice_read" << p->name() << "(const ::Ice::InputStreamPtr&, " << name
                  << "&);";
                H.zeroIndent();
                H << nl << "#endif";
                H.restoreIndent();
            }

            C << sp << nl << "void" << nl << scope.substr(2) << "__write" << name
              << "(::IceInternal::BasicStream* __os, const " << s << "* begin, const " << s << "* end)";
            C << sb;
            C << nl << "::Ice::Int size = static_cast< ::Ice::Int>(end - begin);";
            C << nl << "__os->writeSize(size);";
            C << nl << "for(int i = 0; i < size; ++i)";
            C << sb;
            writeMarshalUnmarshalCode(C, type, "begin[i]", true);
            C << eb;
            C << eb;

            C << sp << nl << "void" << nl << scope.substr(2) << "__read" << name
              << "(::IceInternal::BasicStream* __is, " << scoped << "& v)";
            C << sb;
            C << nl << "::Ice::Int sz;";
            C << nl << "__is->readSize(sz);";
            if(type->isVariableLength())
            {
                // Protect against bogus sequence sizes.
                C << nl << "__is->startSeq(sz, " << type->minWireSize() << ");";
            }
            else
            {
                C << nl << "__is->checkFixedSeq(sz, " << type->minWireSize() << ");";
            }
            C << nl << "v.resize(sz);";
            C << nl << "for(int i = 0; i < sz; ++i)";
            C << sb;
            writeMarshalUnmarshalCode(C, type, "v[i]", false);

            //
            // After unmarshaling each element, check that there are still enough bytes left in the stream
            // to unmarshal the remainder of the sequence, and decrement the count of elements
            // yet to be unmarshaled for sequences with variable-length element type (that is, for sequences
            // of classes, structs, dictionaries, sequences, strings, or proxies). This allows us to
            // abort unmarshaling for bogus sequence sizes at the earliest possible moment.
            // (For fixed-length sequences, we don't need to do this because the prediction of how many
            // bytes will be taken up by the sequence is accurate.)
            //
            if(type->isVariableLength())
            {
                if(!SequencePtr::dynamicCast(type))
                {
                    //
                    // No need to check for directly nested sequences because, at the start of each
                    // sequence, we check anyway.
                    //
                    C << nl << "__is->checkSeq();";
                }
                C << nl << "__is->endElement();";
            }
            C << eb;
            if(type->isVariableLength())
            {
                C << nl << "__is->endSeq(sz);";
            }
            C << eb;

            if(_stream)
            {
                C.zeroIndent();
                C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
                C.restoreIndent();
                C << nl << "//Stream api is not supported in VC++ 6";
                C.zeroIndent();
                C << nl << "#else";
                C.restoreIndent();
                C << sp << nl << "void" << nl << scope.substr(2) << "ice_write" << p->name()
                  << "(const ::Ice::OutputStreamPtr& __outS, const " << scoped << "& v)";
                C << sb;
                C << nl << "__outS->writeSize(::Ice::Int(v.size()));";
                C << nl << scoped << "::const_iterator p;";
                C << nl << "for(p = v.begin(); p != v.end(); ++p)";
                C << sb;
                writeStreamMarshalUnmarshalCode(C, type, "(*p)", true, "", StringList(), _useWstring);
                C << eb;
                C << eb;

                C << sp << nl << "void" << nl << scope.substr(2) << "ice_read" << p->name()
                  << "(const ::Ice::InputStreamPtr& __inS, " << scoped << "& v)";
                C << sb;
                C << nl << "::Ice::Int sz = __inS->readSize();";
                C << nl << "v.resize(sz);";
                C << nl << "for(int i = 0; i < sz; ++i)";
                C << sb;
                writeStreamMarshalUnmarshalCode(C, type, "v[i]", false, "", StringList(), _useWstring);
                C << eb;
                C << eb;
                C.zeroIndent();
                C << nl << "#endif";
                C.restoreIndent();
            }
        }
    }
}

void
Slice::Gen::TypesVisitor::visitDictionary(const DictionaryPtr& p)
{
    string name = fixKwd(p->name());
    TypePtr keyType = p->keyType();
    if(SequencePtr::dynamicCast(keyType))
    {
        SequencePtr s = SequencePtr::dynamicCast(keyType);
        BuiltinPtr builtin = BuiltinPtr::dynamicCast(s->type());
        if(builtin && builtin->kind() == Builtin::KindByte)
        {
            StringList metaData = s->getMetaData();
            bool protobuf;
            findMetaData(s, metaData, protobuf);
            if(protobuf)
            {
                emitWarning(p->file(), p->line(), "protobuf cannot be used as a dictionary key in C++");
            }
        }
    }

    TypePtr valueType = p->valueType();
    string ks = typeToString(keyType, p->keyMetaData(), _useWstring);
    if(ks[0] == ':')
    {
        ks.insert(0, " ");
    }
    string vs = typeToString(valueType, p->valueMetaData(), _useWstring);
    H << sp << nl << "typedef ::std::map<" << ks << ", " << vs << "> " << name << ';';

    if(!p->isLocal())
    {
        string scoped = fixKwd(p->scoped());
        string scope = fixKwd(p->scope());

        H << nl << _dllExport << "void __write" << name << "(::IceInternal::BasicStream*, const " << name << "&);";
        H << nl << _dllExport << "void __read" << name << "(::IceInternal::BasicStream*, " << name << "&);";

        if(_stream)
        {
            H.zeroIndent();
            H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            H.restoreIndent();
            H << nl << "//Stream api is not supported in VC++ 6";
            H.zeroIndent();
            H << nl << "#else";
            H.restoreIndent();
            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_write" << p->name() << "(const ::Ice::OutputStreamPtr&, const " << name
              << "&);";
            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_read" << p->name() << "(const ::Ice::InputStreamPtr&, " << name << "&);";
            H.zeroIndent();
            H << nl << "#endif";
            H.restoreIndent();
        }

        C << sp << nl << "void" << nl << scope.substr(2) << "__write" << name
          << "(::IceInternal::BasicStream* __os, const " << scoped << "& v)";
        C << sb;
        C << nl << "__os->writeSize(::Ice::Int(v.size()));";
        C << nl << scoped << "::const_iterator p;";
        C << nl << "for(p = v.begin(); p != v.end(); ++p)";
        C << sb;
        writeMarshalUnmarshalCode(C, keyType, "p->first", true);
        writeMarshalUnmarshalCode(C, valueType, "p->second", true);
        C << eb;
        C << eb;

        C << sp << nl << "void" << nl << scope.substr(2) << "__read" << name
          << "(::IceInternal::BasicStream* __is, " << scoped << "& v)";
        C << sb;
        C << nl << "::Ice::Int sz;";
        C << nl << "__is->readSize(sz);";
        C << nl << "while(sz--)";
        C << sb;
        C << nl << "::std::pair<const " << ks << ", " << vs << "> pair;";
        const string pf = string("const_cast<") + ks + "&>(pair.first)";
        writeMarshalUnmarshalCode(C, keyType, pf, false);
        C << nl << scoped << "::iterator __i = v.insert(v.end(), pair);";
        writeMarshalUnmarshalCode(C, valueType, "__i->second", false);
        C << eb;
        C << eb;

        if(_stream)
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp << nl << "void" << nl << scope.substr(2) << "ice_write" << p->name()
              << "(const ::Ice::OutputStreamPtr& __outS, const " << scoped << "& v)";
            C << sb;
            C << nl << "__outS->writeSize(::Ice::Int(v.size()));";
            C << nl << scoped << "::const_iterator p;";
            C << nl << "for(p = v.begin(); p != v.end(); ++p)";
            C << sb;
            writeStreamMarshalUnmarshalCode(C, keyType, "p->first", true, "", p->keyMetaData(), _useWstring);
            writeStreamMarshalUnmarshalCode(C, valueType, "p->second", true, "", p->valueMetaData(), _useWstring);
            C << eb;
            C << eb;

            C << sp << nl << "void" << nl << scope.substr(2) << "ice_read" << p->name()
              << "(const ::Ice::InputStreamPtr& __inS, " << scoped << "& v)";
            C << sb;
            C << nl << "::Ice::Int sz = __inS->readSize();";
            C << nl << "while(sz--)";
            C << sb;
            C << nl << "::std::pair<const " << ks << ", " << vs << "> pair;";
            writeStreamMarshalUnmarshalCode(C, keyType, pf, false, "", p->keyMetaData(), _useWstring);
            C << nl << scoped << "::iterator __i = v.insert(v.end(), pair);";
            writeStreamMarshalUnmarshalCode(C, valueType, "__i->second", false, "", p->valueMetaData(), _useWstring);
            C << eb;
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
    }
}

void
Slice::Gen::TypesVisitor::visitEnum(const EnumPtr& p)
{
    string name = fixKwd(p->name());
    EnumeratorList enumerators = p->getEnumerators();
    H << sp << nl << "enum " << name;
    H << sb;
    EnumeratorList::const_iterator en = enumerators.begin();
    while(en != enumerators.end())
    {
        H << nl << fixKwd((*en)->name());
        if(++en != enumerators.end())
        {
            H << ',';
        }
    }
    H << eb << ';';

    if(!p->isLocal())
    {
        string scoped = fixKwd(p->scoped());
        string scope = fixKwd(p->scope());

        size_t sz = enumerators.size();
        assert(sz <= 0x7fffffff); // 64-bit enums are not supported

        H << sp << nl << _dllExport << "void __write(::IceInternal::BasicStream*, " << name << ");";
        H << nl << _dllExport << "void __read(::IceInternal::BasicStream*, " << name << "&);";

        if(_stream)
        {
            H.zeroIndent();
            H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            H.restoreIndent();
            H << nl << "//Stream api is not supported in VC++ 6";
            H.zeroIndent();
            H.restoreIndent();
            H << nl << "#else";
            H << sp << nl << _dllExport << "ICE_DEPRECATED_API void ice_write" << p->name()
              << "(const ::Ice::OutputStreamPtr&, " << name << ");";
            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_read" << p->name() << "(const ::Ice::InputStreamPtr&, " << name << "&);";
            H.zeroIndent();
            H << nl << "#endif";
            H.restoreIndent();
        }

        C << sp << nl << "void" << nl << scope.substr(2) << "__write(::IceInternal::BasicStream* __os, " << scoped
          << " v)";
        C << sb;
        if(sz <= 0x7f)
        {
            C << nl << "__os->write(static_cast< ::Ice::Byte>(v), " << sz << ");";
        }
        else if(sz <= 0x7fff)
        {
            C << nl << "__os->write(static_cast< ::Ice::Short>(v), " << sz << ");";
        }
        else
        {
            C << nl << "__os->write(static_cast< ::Ice::Int>(v), " << sz << ");";
        }
        C << eb;

        C << sp << nl << "void" << nl << scope.substr(2) << "__read(::IceInternal::BasicStream* __is, " << scoped
          << "& v)";
        C << sb;
        if(sz <= 0x7f)
        {
            C << nl << "::Ice::Byte val;";
        }
        else if(sz <= 0x7fff)
        {
            C << nl << "::Ice::Short val;";
        }
        else
        {
            C << nl << "::Ice::Int val;";
        }
        C << nl << "__is->read(val, " << sz << ");";
        C << nl << "v = static_cast< " << scoped << ">(val);";
        C << eb;

        if(_stream)
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C.restoreIndent();
            C << nl << "#else";
            C << sp << nl << "void" << nl << scope.substr(2) << "ice_write" << p->name()
              << "(const ::Ice::OutputStreamPtr& __outS, " << scoped << " v)";
            C << sb;
            C << nl << "if(";
            if(sz > 0x7f)
            {
                C << "static_cast<int>(val) < 0 || ";
            }
            C << "static_cast<int>(v) >= " << sz << ")";
            C << sb;
            C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"enumerator out of range\");";
            C << eb;
            if(sz <= 0x7f)
            {
                C << nl << "__outS->writeByte(static_cast< ::Ice::Byte>(v));";
            }
            else if(sz <= 0x7fff)
            {
                C << nl << "__outS->writeShort(static_cast< ::Ice::Short>(v));";
            }
            else
            {
                C << nl << "__outS->writeInt(static_cast< ::Ice::Int>(v));";
            }
            C << eb;

            C << sp << nl << "void" << nl << scope.substr(2) << "ice_read" << p->name()
              << "(const ::Ice::InputStreamPtr& __inS, " << scoped << "& v)";
            C << sb;
            if(sz <= 0x7f)
            {
                C << nl << "::Ice::Byte val = __inS->readByte();";
            }
            else if(sz <= 0x7fff)
            {
                C << nl << "::Ice::Short val = __inS->readShort();";
            }
            else
            {
                C << nl << "::Ice::Int val = __inS->readInt();";
            }
            C << nl << "if(";
            if(sz > 0x7f)
            {
                C << "val < 0 || ";
            }
            C << "val > " << sz << ")";
            C << sb;
            C << nl << "throw ::Ice::MarshalException(__FILE__, __LINE__, \"enumerator out of range\");";
            C << eb;
            C << nl << "v = static_cast< " << scoped << ">(val);";
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
    }
}

void
Slice::Gen::TypesVisitor::visitConst(const ConstPtr& p)
{
    H << sp;
    H << nl << "const " << typeToString(p->type(), p->typeMetaData(), _useWstring) << " " << fixKwd(p->name())
      << " = ";

    BuiltinPtr bp = BuiltinPtr::dynamicCast(p->type());
    if(bp && bp->kind() == Builtin::KindString)
    {
        //
        // Expand strings into the basic source character set. We can't use isalpha() and the like
        // here because they are sensitive to the current locale.
        //
        static const string basicSourceChars = "abcdefghijklmnopqrstuvwxyz"
                                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                               "0123456789"
                                               "_{}[]#()<>%:;.?*+-/^&|~!=,\\\"' ";
        static const set<char> charSet(basicSourceChars.begin(), basicSourceChars.end());

        if(_useWstring || findMetaData(p->typeMetaData()) == "wstring")
        {
            H << 'L';
        }
        H << "\"";                                      // Opening "

        const string val = p->value();
        for(string::const_iterator c = val.begin(); c != val.end(); ++c)
        {
            if(charSet.find(*c) == charSet.end())
            {
                unsigned char uc = *c;                  // char may be signed, so make it positive
                ostringstream s;
                s << "\\";                              // Print as octal if not in basic source character set
                s.width(3);
                s.fill('0');
                s << oct;
                s << static_cast<unsigned>(uc);
                H << s.str();
            }
            else
            {
                H << *c;                                // Print normally if in basic source character set
            }
        }

        H << "\"";                                      // Closing "
    }
    else if(bp && bp->kind() == Builtin::KindLong)
    {
        H << "ICE_INT64(" << p->value() << ")";
    }
    else
    {
        EnumPtr ep = EnumPtr::dynamicCast(p->type());
        if(ep)
        {
            H << fixKwd(p->value());
        }
        else
        {
            H << p->value();
        }
    }

    H << ';';
}

void
Slice::Gen::TypesVisitor::emitUpcall(const ExceptionPtr& base, const string& call, bool isLocal)
{
    C.zeroIndent();
    C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    C.restoreIndent();
    C << nl << (base ? fixKwd(base->name()) : string(isLocal ? "LocalException" : "UserException")) << call;
    C.zeroIndent();
    C << nl << "#else";
    C.restoreIndent();
    C << nl << (base ? fixKwd(base->scoped()) : string(isLocal ? "::Ice::LocalException" : "::Ice::UserException"))
      << call;
    C.zeroIndent();
    C << nl << "#endif";
    C.restoreIndent();
}

Slice::Gen::ProxyDeclVisitor::ProxyDeclVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::ProxyDeclVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasNonLocalClassDecls())
    {
        return false;
    }

    H << sp << nl << "namespace IceProxy" << nl << '{';

    return true;
}

void
Slice::Gen::ProxyDeclVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::ProxyDeclVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDecls())
    {
        return false;
    }

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ProxyDeclVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

void
Slice::Gen::ProxyDeclVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    if(p->isLocal())
    {
        return;
    }

    string name = fixKwd(p->name());

    H << sp << nl << "class " << name << ';';
}

Slice::Gen::ProxyVisitor::ProxyVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::ProxyVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    H << sp << nl << "namespace IceProxy" << nl << '{';

    return true;
}

void
Slice::Gen::ProxyVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::ProxyVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ProxyVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::ProxyVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    ClassList bases = p->bases();

    H << sp << nl << "class " << name << " : ";
    if(bases.empty())
    {
        H << "virtual public ::IceProxy::Ice::Object";
    }
    else
    {
        H.useCurrentPosAsIndent();
        ClassList::const_iterator q = bases.begin();
        while(q != bases.end())
        {
            H << "virtual public ::IceProxy" << fixKwd((*q)->scoped());
            if(++q != bases.end())
            {
                H << ',' << nl;
            }
        }
        H.restoreIndent();
    }

    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    if(_dllExport != "")
    {
	//
	// To export the virtual table
	//
	C << nl << "#ifdef __SUNPRO_CC";
	C << nl << "class "
	    << (_dllExport.empty() ? "" : "ICE_DECLSPEC_EXPORT ")
	    << "IceProxy" << scoped << ";";
	C << nl << "#endif";
    }

    return true;
}

void
Slice::Gen::ProxyVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    string scope = fixKwd(p->scope());

    //
    // "Overwrite" various non-virtual functions in ::IceProxy::Ice::Object that return an ObjectPrx and
    // are more usable when they return a <name>Prx
    //

    //
    // No identity!
    //

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_context(const ::Ice::Context& __context) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_context(__context).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_context(__context).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    //
    // No facet!
    //

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_adapterId(const std::string& __id) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_adapterId(__id).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_adapterId(__id).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_endpoints(const ::Ice::EndpointSeq& __endpoints) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_endpoints(__endpoints).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_endpoints(__endpoints).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_locatorCacheTimeout(int __timeout) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_locatorCacheTimeout(__timeout).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_locatorCacheTimeout(__timeout).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_connectionCached(bool __cached) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_connectionCached(__cached).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_connectionCached(__cached).get());";
    H.dec(); H << nl << "#endif"; H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_endpointSelection(::Ice::EndpointSelectionType __est) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_endpointSelection(__est).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_endpointSelection(__est).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_secure(bool __secure) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_secure(__secure).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_secure(__secure).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_preferSecure(bool __preferSecure) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_preferSecure(__preferSecure).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_preferSecure(__preferSecure).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_router(const ::Ice::RouterPrx& __router) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_router(__router).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_router(__router).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_locator(const ::Ice::LocatorPrx& __locator) const";
    H << sb;
    H.dec(); H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_locator(__locator).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_locator(__locator).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_collocationOptimized(bool __co) const";
    H << sb;
    H.dec(); H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_collocationOptimized(__co).get());";
    H.dec(); H << nl << "#else"; H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_collocationOptimized(__co).get());";
    H.dec(); H << nl << "#endif"; H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_twoway() const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_twoway().get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_twoway().get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_oneway() const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_oneway().get());";
    H.dec();
    H << nl << "#else"; H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_oneway().get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_batchOneway() const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_batchOneway().get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_batchOneway().get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_datagram() const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_datagram().get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_datagram().get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_batchDatagram() const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_batchDatagram().get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_batchDatagram().get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_compress(bool __compress) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_compress(__compress).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_compress(__compress).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_timeout(int __timeout) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_timeout(__timeout).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_timeout(__timeout).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << "::IceInternal::ProxyHandle<" << name << "> ice_connectionId(const std::string& __id) const";
    H << sb;
    H.dec();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H.inc();
    H << nl << "typedef ::IceProxy::Ice::Object _Base;";
    H << nl << "return dynamic_cast<" << name << "*>(_Base::ice_connectionId(__id).get());";
    H.dec();
    H << nl << "#else";
    H.inc();
    H << nl << "return dynamic_cast<" << name << "*>(::IceProxy::Ice::Object::ice_connectionId(__id).get());";
    H.dec();
    H << nl << "#endif";
    H.inc();
    H << eb;

    H << nl << nl << _dllExport << "static const ::std::string& ice_staticId();";

    H.dec();
    H << sp << nl << "private: ";
    H.inc();
    H << sp << nl << _dllExport << "virtual ::IceInternal::Handle< ::IceDelegateM::Ice::Object> __createDelegateM();";
    H << nl <<  _dllExport << "virtual ::IceInternal::Handle< ::IceDelegateD::Ice::Object> __createDelegateD();";
    H << nl <<  _dllExport << "virtual ::IceProxy::Ice::Object* __newInstance() const;";
    H << eb << ';';

    C << sp;
    C << nl << "const ::std::string&" << nl << "IceProxy" << scoped << "::ice_staticId()";
    C << sb;
    C << nl << "return "<< scoped << "::ice_staticId();";
    C << eb;

    C << sp << nl << "::IceInternal::Handle< ::IceDelegateM::Ice::Object>";
    C << nl << "IceProxy" << scoped << "::__createDelegateM()";
    C << sb;
    C << nl << "return ::IceInternal::Handle< ::IceDelegateM::Ice::Object>(new ::IceDelegateM" << scoped << ");";
    C << eb;
    C << sp << nl << "::IceInternal::Handle< ::IceDelegateD::Ice::Object>";
    C << nl << "IceProxy" << scoped << "::__createDelegateD()";
    C << sb;
    C << nl << "return ::IceInternal::Handle< ::IceDelegateD::Ice::Object>(new ::IceDelegateD" << scoped << ");";
    C << eb;
    C << sp << nl << "::IceProxy::Ice::Object*";
    C << nl << "IceProxy" << scoped << "::__newInstance() const";
    C << sb;
    C << nl << "return new " << name << ";";
    C << eb;

    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::ProxyVisitor::visitOperation(const OperationPtr& p)
{
    string name = p->name();
    string scoped = fixKwd(p->scoped());
    string scope = fixKwd(p->scope());

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret, p->getMetaData(), _useWstring | TypeContextAMIEnd);
    string retSEndAMI = returnTypeToString(ret, p->getMetaData(), _useWstring | TypeContextAMIPrivateEnd);

    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);
    string clScope = fixKwd(cl->scope());
    string delName = "Callback_" + cl->name() + "_" + name;
    string delNameScoped = clScope + delName;

    vector<string> params;
    vector<string> paramsDecl;
    vector<string> args;

    vector<string> paramsAMI;
    vector<string> paramsDeclAMI;
    vector<string> argsAMI;
    vector<string> outParamsAMI;
    vector<string> outParamsDeclAMI;
    vector<string> outParamsDeclEndAMI;

    ParamDeclList paramList = p->parameters();
    ParamDeclList inParams;
    ParamDeclList outParams;
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        string paramName = fixKwd((*q)->name());

        StringList metaData = (*q)->getMetaData();
        string typeString;
        string typeStringEndAMI;
        if((*q)->isOutParam())
        {
            typeString = outputTypeToString((*q)->type(), metaData, _useWstring | TypeContextAMIEnd);
            typeStringEndAMI = outputTypeToString((*q)->type(), metaData, _useWstring | TypeContextAMIPrivateEnd);
        }
        else
        {
            typeString = inputTypeToString((*q)->type(), metaData, _useWstring);
        }

        params.push_back(typeString);
        paramsDecl.push_back(typeString + ' ' + paramName);
        args.push_back(paramName);

        if(!(*q)->isOutParam())
        {
            paramsAMI.push_back(typeString);
            paramsDeclAMI.push_back(typeString + ' ' + paramName);
            argsAMI.push_back(paramName);
	    inParams.push_back(*q);
        }
	else
	{
            outParamsAMI.push_back(typeString);
	    outParamsDeclAMI.push_back(typeString + ' ' + paramName);
	    outParamsDeclEndAMI.push_back(typeStringEndAMI + ' ' + paramName);
	    outParams.push_back(*q);
	}
    }

    //
    // Check if we need to generate a private ___end_ method. This is the case if the
    // when using certain mapping features such as cpp:array or cpp:range:array. While
    // the regular end_ method can't return pair<const TYPE*, const TYPE*> because the 
    // pointers would be invalid once end_ returns, we still want to allow using this
    // alternate mapping with AMI response callbacks (to allow zero-copy for instance).
    // For this purpose, we generate a special ___end method which is used by the 
    // __completed implementation of the generated Callback_Inft_opName operation 
    // delegate.
    //
    bool generatePrivateEnd = retS != retSEndAMI || outParamsDeclAMI != outParamsDeclEndAMI;
    if(generatePrivateEnd)
    {
        string typeStringEndAMI = outputTypeToString(ret, p->getMetaData(), _useWstring | TypeContextAMIPrivateEnd);
        outParamsDeclEndAMI.push_back(typeStringEndAMI + ' ' + "__ret");
    }

    string thisPointer = fixKwd(scope.substr(0, scope.size() - 2)) + "*";

    string deprecateSymbol = getDeprecateSymbol(p, cl);
    H << sp << nl << deprecateSymbol << retS << ' ' << fixKwd(name) << spar << paramsDecl << epar;
    H << sb;
    H << nl;
    if(ret)
    {
        H << "return ";
    }
    H << fixKwd(name) << spar << args << "0" << epar << ';';
    H << eb;
    H << nl << deprecateSymbol << retS << ' ' << fixKwd(name) << spar << paramsDecl << "const ::Ice::Context& __ctx"
      << epar;
    H << sb;
    H << nl;
    if(ret)
    {
        H << "return ";
    }
    H << fixKwd(name) << spar << args << "&__ctx" << epar << ';';
    H << eb;

    H << sp << nl << "::Ice::AsyncResultPtr begin_" << name << spar << paramsDeclAMI << epar;
    H << sb;
    H << nl << "return begin_" << name << spar << argsAMI << "0" << "::IceInternal::__dummyCallback" << "0" 
      << epar << ';';
    H << eb;

    H << sp << nl << "::Ice::AsyncResultPtr begin_" << name << spar << paramsDeclAMI
      << "const ::Ice::Context& __ctx" << epar;
    H << sb;
    H << nl << "return begin_" << name << spar << argsAMI << "&__ctx" << "::IceInternal::__dummyCallback" << "0"
      << epar << ';';
    H << eb;

    H << sp << nl << "::Ice::AsyncResultPtr begin_" << name << spar << paramsDeclAMI
      << "const ::Ice::CallbackPtr& __del"
      << "const ::Ice::LocalObjectPtr& __cookie = 0" << epar;
    H << sb;
    H << nl << "return begin_" << name << spar << argsAMI << "0" << "__del" << "__cookie" << epar << ';';
    H << eb;

    H << sp << nl << "::Ice::AsyncResultPtr begin_" << name << spar << paramsDeclAMI
      << "const ::Ice::Context& __ctx"
      << "const ::Ice::CallbackPtr& __del"
      << "const ::Ice::LocalObjectPtr& __cookie = 0" << epar;
    H << sb;
    H << nl << "return begin_" << name << spar << argsAMI << "&__ctx" << "__del" << "__cookie" << epar << ';';
    H << eb;

    H << sp << nl << "::Ice::AsyncResultPtr begin_" << name << spar << paramsDeclAMI
      << "const " + delNameScoped + "Ptr& __del"
      << "const ::Ice::LocalObjectPtr& __cookie = 0" << epar;
    H << sb;
    H << nl << "return begin_" << name << spar << argsAMI << "0" << "__del" << "__cookie" << epar << ';';
    H << eb;
    
    H << sp << nl << "::Ice::AsyncResultPtr begin_" << name << spar << paramsDeclAMI
      << "const ::Ice::Context& __ctx"
      << "const " + delNameScoped + "Ptr& __del"
      << "const ::Ice::LocalObjectPtr& __cookie = 0" << epar;
    H << sb;
    H << nl << "return begin_" << name << spar << argsAMI << "&__ctx" << "__del" << "__cookie" << epar << ';';
    H << eb;

    H << sp << nl << retS << " end_" << name << spar << outParamsDeclAMI
      << "const ::Ice::AsyncResultPtr&" << epar << ';';
    if(generatePrivateEnd)
    {
        H << sp << nl << "void ___end_" << name << spar << outParamsDeclEndAMI;
        H << "const ::Ice::AsyncResultPtr&" << epar << ';';
    }

    H << nl;
    H.dec();
    H << nl << "private:";
    H.inc();
    H << sp << nl << _dllExport << retS << ' ' << fixKwd(name) << spar << params << "const ::Ice::Context*" << epar
      << ';';
    H << nl << _dllExport <<  "::Ice::AsyncResultPtr begin_" << name << spar << paramsAMI << "const ::Ice::Context*"
      << "const ::IceInternal::CallbackBasePtr&"
      << "const ::Ice::LocalObjectPtr& __cookie = 0" << epar << ';';
    H << nl;
    H.dec();
    H << nl << "public:";
    H.inc();

    C << sp << nl << retS << nl << "IceProxy" << scoped << spar << paramsDecl << "const ::Ice::Context* __ctx" << epar;
    C << sb;
    C << nl << "int __cnt = 0;";
    C << nl << "while(true)";
    C << sb;
    C << nl << "::IceInternal::Handle< ::IceDelegate::Ice::Object> __delBase;";
    C << nl << "try";
    C << sb;

    if(p->returnsData())
    {
        C << nl << "__checkTwowayOnly(" << p->flattenedScope() + p->name() + "_name);";
    }
    C << nl << "__delBase = __getDelegate(false);";
    C << nl << "::IceDelegate" << thisPointer << " __del = dynamic_cast< ::IceDelegate"
      << thisPointer << ">(__delBase.get());";
    C << nl;
    if(ret)
    {
        C << "return ";
    }
    C << "__del->" << fixKwd(name) << spar << args << "__ctx" << epar << ';';
    if(!ret)
    {
        C << nl << "return;";
    }
    C << eb;
    C << nl << "catch(const ::IceInternal::LocalExceptionWrapper& __ex)";
    C << sb;
    if(p->mode() == Operation::Idempotent || p->mode() == Operation::Nonmutating)
    {
        C << nl << "__handleExceptionWrapperRelaxed(__delBase, __ex, true, __cnt);";
    }
    else
    {
        C << nl << "__handleExceptionWrapper(__delBase, __ex);";
    }
    C << eb;
    C << nl << "catch(const ::Ice::LocalException& __ex)";
    C << sb;
    C << nl << "__handleException(__delBase, __ex, true, __cnt);";
    C << eb;
    C << eb;
    C << eb;

    C << sp << nl << "::Ice::AsyncResultPtr" << nl << "IceProxy" << scope << "begin_" << name << spar << paramsDeclAMI
      << "const ::Ice::Context* __ctx" << "const ::IceInternal::CallbackBasePtr& __del"
      << "const ::Ice::LocalObjectPtr& __cookie" << epar;
    C << sb;
    string flatName = p->flattenedScope() + name + "_name";
    C << nl << "::IceInternal::OutgoingAsyncPtr __result = new ::IceInternal::OutgoingAsync(this, ";
    C << flatName << ", __del, __cookie);";
    C << nl << "try";
    C << sb;
    if(p->returnsData())
    {
	C << nl << "__checkTwowayOnly(" << flatName <<  ");";
    }
    C << nl << "__result->__prepare(" << flatName << ", " << operationModeToString(p->sendMode()) << ", __ctx);";
    C << nl << "::IceInternal::BasicStream* __os = __result->__getOs();";
    writeMarshalCode(C, inParams, 0, StringList(), true);
    if(p->sendsClasses())
    {
	C << nl << "__os->writePendingObjects();";
    }
    C << nl << "__os->endWriteEncaps();";
    C << nl << "__result->__send(true);";
    C << eb;
    C << nl << "catch(const ::Ice::LocalException& __ex)";
    C << sb;
    C << nl << "__result->__exceptionAsync(__ex);";
    C << eb;
    C << nl << "return __result;";
    C << eb;

    C << sp << nl << retS << nl << "IceProxy" << scope << "end_" << name << spar << outParamsDeclAMI
      << "const ::Ice::AsyncResultPtr& __result" << epar;
    C << sb;
    if(p->returnsData())
    {
        C << nl << "::Ice::AsyncResult::__check(__result, this, " << flatName << ");";
        C << nl << "if(!__result->__wait())";
        C << sb;
        C << nl << "try";
        C << sb;
        C << nl << "__result->__throwUserException();";
        C << eb;
        //
        // Generate a catch block for each legal user exception.
        // (See comment in DelegateMVisitor::visitOperation() for details.)
        //
        ExceptionList throws = p->throws();
        throws.sort();
        throws.unique();
#if defined(__SUNPRO_CC)
        throws.sort(derivedToBaseCompare);
#else
        throws.sort(Slice::DerivedToBaseCompare());
#endif
        for(ExceptionList::const_iterator i = throws.begin(); i != throws.end(); ++i)
        {
            string scoped = (*i)->scoped();
            C << nl << "catch(const " << fixKwd((*i)->scoped()) << "&)";
            C << sb;
            C << nl << "throw;";
            C << eb;
        }
        C << nl << "catch(const ::Ice::UserException& __ex)";
        C << sb;
        C << nl << "throw ::Ice::UnknownUserException(__FILE__, __LINE__, __ex.ice_name());";
        C << eb;
        C << eb;
        writeAllocateCode(C, ParamDeclList(), ret, p->getMetaData(), _useWstring | TypeContextAMIEnd);
        C << nl << "::IceInternal::BasicStream* __is = __result->__getIs();";
        if(ret || !outParams.empty())
        {
            C << nl << "__is->startReadEncaps();";
            writeUnmarshalCode(C, outParams, ret, p->getMetaData(), _useWstring | TypeContextAMIEnd);
            if(p->returnsClasses())
            {
                C << nl << "__is->readPendingObjects();";
            }
            C << nl << "__is->endReadEncaps();";
        }
        else
        {
            C << nl << "__is->skipEmptyEncaps();";
        }
        if(ret)
        {
            C << nl << "return __ret;";
        }
    }
    else
    {
        C << nl << "__end(__result, " << flatName << ");";
    }
    C << eb;

    if(generatePrivateEnd)
    {
        assert(p->returnsData());

        C << sp << nl << "void IceProxy" << scope << "___end_" << name << spar << outParamsDeclEndAMI
          << "const ::Ice::AsyncResultPtr& __result" << epar;
        C << sb;
        C << nl << "::Ice::AsyncResult::__check(__result, this, " << flatName << ");";
        C << nl << "if(!__result->__wait())";
        C << sb;
        C << nl << "try";
        C << sb;
        C << nl << "__result->__throwUserException();";
        C << eb;
        //
        // Generate a catch block for each legal user exception.
        // (See comment in DelegateMVisitor::visitOperation() for details.)
        //
        ExceptionList throws = p->throws();
        throws.sort();
        throws.unique();
#if defined(__SUNPRO_CC)
        throws.sort(derivedToBaseCompare);
#else
        throws.sort(Slice::DerivedToBaseCompare());
#endif
        for(ExceptionList::const_iterator i = throws.begin(); i != throws.end(); ++i)
        {
            string scoped = (*i)->scoped();
            C << nl << "catch(const " << fixKwd((*i)->scoped()) << "&)";
            C << sb;
            C << nl << "throw;";
            C << eb;
        }
        C << nl << "catch(const ::Ice::UserException& __ex)";
        C << sb;
        C << nl << "throw ::Ice::UnknownUserException(__FILE__, __LINE__, __ex.ice_name());";
        C << eb;
        C << eb;
        C << nl << "::IceInternal::BasicStream* __is = __result->__getIs();";
        if(ret || !outParams.empty())
        {
            C << nl << "__is->startReadEncaps();";
            writeUnmarshalCode(C, outParams, ret, p->getMetaData(), _useWstring | TypeContextAMIPrivateEnd);
            if(p->returnsClasses())
            {
                C << nl << "__is->readPendingObjects();";
            }
            C << nl << "__is->endReadEncaps();";
        }
        else
        {
            C << nl << "__is->skipEmptyEncaps();";
        }
        C << eb;
    }

    if(cl->hasMetaData("ami") || p->hasMetaData("ami"))
    {
        string classNameAMI = "AMI_" + cl->name();
        string classScope = fixKwd(cl->scope());
        string classScopedAMI = classScope + classNameAMI;
        string opScopedAMI = classScopedAMI + "_" + name;

        H << nl << _dllExport << "bool " << name << "_async" << spar << ("const " + opScopedAMI + "Ptr&")
          << paramsAMI << epar << ';';
        H << nl << _dllExport << "bool " << name << "_async" << spar << ("const " + opScopedAMI + "Ptr&")
          << paramsAMI << "const ::Ice::Context&" << epar << ';';

        C << sp << nl << "bool" << nl << "IceProxy" << scope << name << "_async" << spar
          << ("const " + opScopedAMI + "Ptr& __cb") << paramsDeclAMI << epar;
        C << sb;
        if(p->returnsData())
        {
            C << nl << delNameScoped << "Ptr __del;";
            C << nl << "if(dynamic_cast< ::Ice::AMISentCallback*>(__cb.get()))";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception, &" << opScopedAMI << "::__sent);";
            C << eb;
            C << nl << "else";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception);";
            C << eb;
        }
        else
        {
            C << nl << "::IceInternal::CallbackBasePtr __del;";
            C << nl << "if(dynamic_cast< ::Ice::AMISentCallback*>(__cb.get()))";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception, &" << opScopedAMI << "::__sent);";
            C << eb;
            C << nl << "else";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception);";
            C << eb;
        }
        C << nl << "::Ice::AsyncResultPtr __ar = begin_" << name << spar << argsAMI << "0, __del" << epar << ';';
        C << nl << "return __ar->isSentSynchronously();";
        C << eb;

        C << sp << nl << "bool" << nl << "IceProxy" << scope << name << "_async" << spar
          << ("const " + opScopedAMI + "Ptr& __cb") << paramsDeclAMI << "const ::Ice::Context& __ctx"
          << epar;
        C << sb;
        if(p->returnsData())
        {
            C << nl << delNameScoped << "Ptr __del;";
            C << nl << "if(dynamic_cast< ::Ice::AMISentCallback*>(__cb.get()))";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception, &" << opScopedAMI << "::__sent);";
            C << eb;
            C << nl << "else";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception);";
            C << eb;
        }
        else
        {
            C << nl << "::IceInternal::CallbackBasePtr __del;";
            C << nl << "if(dynamic_cast< ::Ice::AMISentCallback*>(__cb.get()))";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception, &" << opScopedAMI << "::__sent);";
            C << eb;
            C << nl << "else";
            C << sb;
            C << nl << " __del = " << classScope << "new" << delName << "(__cb, &" << opScopedAMI << "::__response, &"
              << opScopedAMI << "::__exception);";
            C << eb;
        }
        C << nl << "::Ice::AsyncResultPtr __ar = begin_" << name << spar << argsAMI << "&__ctx" << "__del" << epar
          << ';';
        C << nl << "return __ar->isSentSynchronously();";
        C << eb;
    }
}

Slice::Gen::DelegateVisitor::DelegateVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::DelegateVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    H << sp << nl << "namespace IceDelegate" << nl << '{';

    return true;
}

void
Slice::Gen::DelegateVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::DelegateVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::DelegateVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::DelegateVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    if(bases.empty())
    {
        H << "virtual public ::IceDelegate::Ice::Object";
    }
    else
    {
        H.useCurrentPosAsIndent();
        ClassList::const_iterator q = bases.begin();
        while(q != bases.end())
        {
            H << "virtual public ::IceDelegate" << fixKwd((*q)->scoped());
            if(++q != bases.end())
            {
                H << ',' << nl;
            }
        }
        H.restoreIndent();
    }
    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    return true;
}

void
Slice::Gen::DelegateVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    H << eb << ';';

    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::DelegateVisitor::visitOperation(const OperationPtr& p)
{
    string name = fixKwd(p->name());

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret, p->getMetaData(), _useWstring);

    vector<string> params;

    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        StringList metaData = (*q)->getMetaData();
#if defined(__SUNPRO_CC) && (__SUNPRO_CC==0x550)
        //
        // Work around for Sun CC 5.5 bug #4853566
        //
        string typeString;
        if((*q)->isOutParam())
        {
            typeString = outputTypeToString((*q)->type(), metaData, _useWstring);
        }
        else
        {
            typeString = inputTypeToString((*q)->type(), metaData, _useWstring);
        }
#else
        string typeString = (*q)->isOutParam() ?  outputTypeToString((*q)->type(), metaData, _useWstring)
            : inputTypeToString((*q)->type(), metaData, _useWstring);
#endif

        params.push_back(typeString);
    }

    params.push_back("const ::Ice::Context*");

    H << sp << nl << "virtual " << retS << ' ' << name << spar << params << epar << " = 0;";
}

Slice::Gen::DelegateMVisitor::DelegateMVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::DelegateMVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    H << sp << nl << "namespace IceDelegateM" << nl << '{';

    return true;
}

void
Slice::Gen::DelegateMVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::DelegateMVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::DelegateMVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::DelegateMVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    H.useCurrentPosAsIndent();
    H << "virtual public ::IceDelegate" << scoped << ',';
    if(bases.empty())
    {
        H << nl << "virtual public ::IceDelegateM::Ice::Object";
    }
    else
    {
        ClassList::const_iterator q = bases.begin();
        while(q != bases.end())
        {
            H << nl << "virtual public ::IceDelegateM" << fixKwd((*q)->scoped());
            if(++q != bases.end())
            {
                H << ',';
            }
        }
    }
    H.restoreIndent();
    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    return true;
}

void
Slice::Gen::DelegateMVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    H << eb << ';';

    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::DelegateMVisitor::visitOperation(const OperationPtr& p)
{
    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret, p->getMetaData(), _useWstring);

    vector<string> params;
    vector<string> paramsDecl;

    ParamDeclList inParams;
    ParamDeclList outParams;
    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        string paramName = fixKwd((*q)->name());
        TypePtr type = (*q)->type();
        bool isOutParam = (*q)->isOutParam();
        StringList metaData = (*q)->getMetaData();
        string typeString;
        if(isOutParam)
        {
            outParams.push_back(*q);
            typeString = outputTypeToString(type, metaData, _useWstring);
        }
        else
        {
            inParams.push_back(*q);
            typeString = inputTypeToString(type, metaData, _useWstring);
        }

        params.push_back(typeString);
        paramsDecl.push_back(typeString + ' ' + paramName);
    }

    params.push_back("const ::Ice::Context*");
    paramsDecl.push_back("const ::Ice::Context* __context");

    string flatName = p->flattenedScope() + p->name() + "_name";

    H << sp << nl << "virtual " << retS << ' ' << name << spar << params << epar << ';';
    C << sp << nl << retS << nl << "IceDelegateM" << scoped << spar << paramsDecl << epar;
    C << sb;
    C << nl << "::IceInternal::Outgoing __og(__handler.get(), " << flatName << ", "
      << operationModeToString(p->sendMode()) << ", __context);";
    if(!inParams.empty())
    {
        C << nl << "try";
        C << sb;
        C << nl << "::IceInternal::BasicStream* __os = __og.os();";
        writeMarshalCode(C, inParams, 0, StringList(), true);
        if(p->sendsClasses())
        {
            C << nl << "__os->writePendingObjects();";
        }
        C << eb;
        C << nl << "catch(const ::Ice::LocalException& __ex)";
        C << sb;
        C << nl << "__og.abort(__ex);";
        C << eb;
    }

    C << nl << "bool __ok = __og.invoke();";

    //
    // Declare the return __ret variable at the top-level scope to
    // enable NRVO with GCC (see also bug #3619).
    //
    writeAllocateCode(C, ParamDeclList(), ret, p->getMetaData(), _useWstring);
    if(!p->returnsData())
    {
        C << nl << "if(!__og.is()->b.empty())";
        C << sb;
    }
    C << nl << "try";
    C << sb;
    C << nl << "if(!__ok)";
    C << sb;
    C << nl << "try";
    C << sb;
    C << nl << "__og.throwUserException();";
    C << eb;
    
    //
    // Generate a catch block for each legal user exception. This is necessary
    // to prevent an "impossible" user exception to be thrown if client and
    // and server use different exception specifications for an operation. For
    // example:
    //
    // Client compiled with:
    // exception A {};
    // exception B {};
    // interface I {
    //     void op() throws A;
    // };
    //
    // Server compiled with:
    // exception A {};
    // exception B {};
    // interface I {
    //     void op() throws B; // Differs from client
    // };
    //
    // We need the catch blocks so, if the server throws B from op(), the
    // client receives UnknownUserException instead of B.
    //
    ExceptionList throws = p->throws();
    throws.sort();
    throws.unique();
#if defined(__SUNPRO_CC)
    throws.sort(derivedToBaseCompare);
#else
    throws.sort(Slice::DerivedToBaseCompare());
#endif
    for(ExceptionList::const_iterator i = throws.begin(); i != throws.end(); ++i)
    {
        C << nl << "catch(const " << fixKwd((*i)->scoped()) << "&)";
        C << sb;
        C << nl << "throw;";
        C << eb;
    }
    C << nl << "catch(const ::Ice::UserException& __ex)";
    C << sb;
    //
    // COMPILERFIX: Don't throw UnknownUserException directly. This is causing access
    // violation errors with Visual C++ 64bits optimized builds. See bug #2962.
    //
    C << nl << "::Ice::UnknownUserException __uue(__FILE__, __LINE__, __ex.ice_name());";
    C << nl << "throw __uue;";
    C << eb;
    C << eb;

    for(ParamDeclList::const_iterator opi = outParams.begin(); opi != outParams.end(); ++opi)
    {
        StructPtr st = StructPtr::dynamicCast((*opi)->type());
        if(st && findMetaData(st->getMetaData()) == "class")
        {
            C << nl << fixKwd((*opi)->name()) << " = new " << fixKwd(st->scoped()) << ";";
        }
    }

    if(ret || !outParams.empty())
    {
        C << nl << "::IceInternal::BasicStream* __is = __og.is();";
        C << nl << "__is->startReadEncaps();";
        writeUnmarshalCode(C, outParams, ret, p->getMetaData());
        if(p->returnsClasses())
        {
            C << nl << "__is->readPendingObjects();";
        }
        C << nl << "__is->endReadEncaps();";
    }
    else
    {
        C << nl << "__og.is()->skipEmptyEncaps();";
    }

    if(ret)
    {
        C << nl << "return __ret;";
    }
    C << eb;
    C << nl << "catch(const ::Ice::LocalException& __ex)";
    C << sb;
    C << nl << "throw ::IceInternal::LocalExceptionWrapper(__ex, false);";
    C << eb;
    if(!p->returnsData())
    {
        C << eb;
    }
    C << eb;
}

Slice::Gen::DelegateDVisitor::DelegateDVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::DelegateDVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    H << sp << nl << "namespace IceDelegateD" << nl << '{';

    return true;
}

void
Slice::Gen::DelegateDVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::DelegateDVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::DelegateDVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::DelegateDVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    ClassList bases = p->bases();

    H << sp << nl << "class " << _dllExport << name << " : ";
    H.useCurrentPosAsIndent();
    H << "virtual public ::IceDelegate" << scoped << ',';
    if(bases.empty())
    {
        H << nl << "virtual public ::IceDelegateD::Ice::Object";
    }
    else
    {
        ClassList::const_iterator q = bases.begin();
        while(q != bases.end())
        {
            H << nl << "virtual public ::IceDelegateD" << fixKwd((*q)->scoped());
            if(++q != bases.end())
            {
                H << ',';
            }
        }
    }
    H.restoreIndent();
    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    return true;
}

void
Slice::Gen::DelegateDVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    H << eb << ';';

    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::DelegateDVisitor::visitOperation(const OperationPtr& p)
{
    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret, p->getMetaData(), _useWstring);

    vector<string> params;
    vector<string> paramsDecl;
    vector<string> args;
    vector<string> argMembers;

    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        string paramName = fixKwd((*q)->name());

        StringList metaData = (*q)->getMetaData();
#if defined(__SUNPRO_CC) && (__SUNPRO_CC==0x550)
        //
        // Work around for Sun CC 5.5 bug #4853566
        //
        string typeString;
        if((*q)->isOutParam())
        {
            typeString = outputTypeToString((*q)->type(), metaData, _useWstring);
        }
        else
        {
            typeString = inputTypeToString((*q)->type(), metaData, _useWstring);
        }
#else
        string typeString = (*q)->isOutParam() ?  outputTypeToString((*q)->type(), metaData, _useWstring)
                                               : inputTypeToString((*q)->type(), metaData, _useWstring);
#endif

        params.push_back(typeString);
        paramsDecl.push_back(typeString + ' ' + paramName);
        args.push_back(paramName);
        argMembers.push_back("_m_" + paramName);
    }

    params.push_back("const ::Ice::Context*");
    args.push_back("__current");
    argMembers.push_back("_current");

    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);
    string thisPointer = fixKwd(cl->scoped()) + "*";

    H << sp;

    H << nl << "virtual " << retS << ' ' << name << spar << params << epar << ';';
    bool amd = !cl->isLocal() && (cl->hasMetaData("amd") || p->hasMetaData("amd"));
    if(amd)
    {
        C << sp << nl << retS << nl << "IceDelegateD" << scoped << spar << params << epar;
        C << sb;
        C << nl << "throw ::Ice::CollocationOptimizationException(__FILE__, __LINE__);";
        if(ret != 0)
        {
            C << nl << "return " << retS << "(); // to avoid a warning with some compilers;";
        }
        C << eb;
    }
    else
    {
        C << sp << nl << retS << nl << "IceDelegateD" << scoped << spar << paramsDecl
          << "const ::Ice::Context* __context" << epar;
        C << sb;
        C << nl << "class _DirectI : public ::IceInternal::Direct";
        C << sb;
        C.dec();
        C << nl << "public:";
        C.inc();

        //
        // Constructor
        //
        C << sp << nl << "_DirectI" << spar;
        if(ret)
        {
            string resultRef = outputTypeToString(ret, p->getMetaData(), _useWstring);
            C << resultRef + " __result";
        }
        C << paramsDecl << "const ::Ice::Current& __current" << epar << " : ";
        C.inc();
        C << nl << "::IceInternal::Direct(__current)";

        if(ret)
        {
            C << "," << nl << "_result(__result)";
        }

        for(size_t i = 0; i < args.size(); ++i)
        {
            if(args[i] != "__current")
            {
                C << "," << nl << argMembers[i] + "(" + args[i] + ")";
            }
        }
        C.dec();
        C << sb;
        C << eb;

        //
        // run
        //
        C << nl << nl << "virtual ::Ice::DispatchStatus";
        C << nl << "run(::Ice::Object* object)";
        C << sb;
        C << nl << thisPointer << " servant = dynamic_cast< " << thisPointer << ">(object);";
        C << nl << "if(!servant)";
        C << sb;
        C << nl << "throw ::Ice::OperationNotExistException(__FILE__, __LINE__, _current.id, _current.facet, _current.operation);";
        C << eb;

        ExceptionList throws = p->throws();

        if(!throws.empty())
        {
            C << nl << "try";
            C << sb;
        }
        C << nl;
        if(ret)
        {
            C << "_result = ";
        }
        C << "servant->" << name << spar << argMembers << epar << ';';
        C << nl << "return ::Ice::DispatchOK;";

        if(!throws.empty())
        {
            C << eb;
            C << nl << "catch(const ::Ice::UserException& __ex)";
            C << sb;
            C << nl << "setUserException(__ex);";
            C << nl << "return ::Ice::DispatchUserException;";
            C << eb;
            throws.sort();
            throws.unique();
#if defined(__SUNPRO_CC)
            throws.sort(derivedToBaseCompare);
#else
            throws.sort(Slice::DerivedToBaseCompare());
#endif

        }

        C << eb;
        C << nl;

        C.dec();
        C << nl << "private:";
        C.inc();
        C << nl;
        if(ret)
        {
            string resultRef= outputTypeToString(ret, p->getMetaData(), _useWstring);
            C << nl << resultRef << " _result;";
        }

        for(size_t j = 0; j < argMembers.size(); ++j)
        {
            if(argMembers[j] != "_current")
            {
                C << nl << params[j] + " " + argMembers[j] << ";";
            }
        }

        C << eb << ";";

        C << nl << nl << "::Ice::Current __current;";
        C << nl << "__initCurrent(__current, " << p->flattenedScope() + p->name() + "_name, "
          << operationModeToString(p->sendMode()) << ", __context);";

        if(ret)
        {
            C << nl << retS << " __result;";
        }

        C << nl << "try";
        C << sb;

        C << nl << "_DirectI __direct" << spar;
        if(ret)
        {
            C << "__result";
        }
        C << args << epar << ";";

        C << nl << "try";
        C << sb;
        C << nl << "__direct.servant()->__collocDispatch(__direct);";
        C << eb;
#if 0
        C << nl << "catch(const ::std::exception& __ex)";
        C << sb;
        C << nl << "__direct.destroy();";
        C << nl << "::IceInternal::LocalExceptionWrapper::throwWrapper(__ex);";
        C << eb;
        C << nl << "catch(...)";
        C << sb;
        C << nl << "__direct.destroy();";
        C << nl << "throw ::Ice::UnknownException(__FILE__, __LINE__, \"unknown c++ exception\");";
        C << eb;
#else
        C << nl << "catch(...)";
        C << sb;
        C << nl << "__direct.destroy();";
        C << nl << "throw;";
        C << eb;
#endif
        C << nl << "__direct.destroy();";
        C << eb;
        for(ExceptionList::const_iterator k = throws.begin(); k != throws.end(); ++k)
        {
            C << nl << "catch(const " << fixKwd((*k)->scoped()) << "&)";
            C << sb;
            C << nl << "throw;";
            C << eb;
        }
        C << nl << "catch(const ::Ice::SystemException&)";
        C << sb;
        C << nl << "throw;";
        C << eb;
        C << nl << "catch(const ::IceInternal::LocalExceptionWrapper&)";
        C << sb;
        C << nl << "throw;";
        C << eb;
        C << nl << "catch(const ::std::exception& __ex)";
        C << sb;
        C << nl << "::IceInternal::LocalExceptionWrapper::throwWrapper(__ex);";
        C << eb;
        C << nl << "catch(...)";
        C << sb;
        C << nl << "throw ::IceInternal::LocalExceptionWrapper("
          << "::Ice::UnknownException(__FILE__, __LINE__, \"unknown c++ exception\"), false);";
        C << eb;
        if(ret)
        {
            C << nl << "return __result;";
        }

        C << eb;
    }
}

Slice::Gen::ObjectDeclVisitor::ObjectDeclVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::ObjectDeclVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasClassDecls())
    {
        return false;
    }

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ObjectDeclVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';
}

void
Slice::Gen::ObjectDeclVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    string name = fixKwd(p->name());

    H << sp << nl << "class " << name << ';';
    H << nl << "bool operator==(const " << name << "&, const " << name << "&);";
    H << nl << "bool operator<(const " << name << "&, const " << name << "&);";
}

void
Slice::Gen::ObjectDeclVisitor::visitOperation(const OperationPtr& p)
{
    string flatName = p->flattenedScope() + p->name() + "_name";
    C << sp << nl << "static const ::std::string " << flatName << " = \"" << p->name() << "\";";
}

Slice::Gen::ObjectVisitor::ObjectVisitor(Output& h, Output& c, const string& dllExport, bool stream) :
    H(h), C(c), _dllExport(dllExport), _stream(stream), _doneStaticSymbol(false), _useWstring(false)
{
}

bool
Slice::Gen::ObjectVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ObjectVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp;
    H << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::ObjectVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());
    string scoped = fixKwd(p->scoped());
    ClassList bases = p->bases();
    ClassDefPtr base;
    if(!bases.empty() && !bases.front()->isInterface())
    {
        base = bases.front();
    }
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList allDataMembers = p->allDataMembers();


    H << sp << nl << "class " << _dllExport << name << " : ";
    H.useCurrentPosAsIndent();
    if(bases.empty())
    {
        if(p->isLocal())
        {
            H << "virtual public ::Ice::LocalObject";
        }
        else
        {
            H << "virtual public ::Ice::Object";
        }
    }
    else
    {
        ClassList::const_iterator q = bases.begin();
        bool virtualInheritance = p->hasMetaData("cpp:virtual") || p->isInterface();
        while(q != bases.end())
        {
            if(virtualInheritance || (*q)->isInterface())
            {
                H << "virtual ";
            }

            H << "public " << fixKwd((*q)->scoped());
            if(++q != bases.end())
            {
                H << ',' << nl;
            }
        }
    }
    H.restoreIndent();
    H << sb;
    H.dec();
    H << nl << "public:" << sp;
    H.inc();

    if(!p->isLocal())
    {
        H << nl << "typedef " << p->name() << "Prx ProxyType;";
    }
    H << nl << "typedef " << p->name() << "Ptr PointerType;";
    H << nl;

    vector<string> params;
    vector<string> allTypes;
    vector<string> allParamDecls;
    DataMemberList::const_iterator q;

    for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
        params.push_back(fixKwd((*q)->name()));
    }

    for(q = allDataMembers.begin(); q != allDataMembers.end(); ++q)
    {
        string typeName = inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
        allTypes.push_back(typeName);
        allParamDecls.push_back(typeName + " __ice_" + (*q)->name());
    }

    if(!p->isInterface())
    {
        H << nl << name << "() {}";
        if(!allParamDecls.empty())
        {
            H << nl;
            if(allParamDecls.size() == 1)
            {
                H << "explicit ";
            }
            H << name << spar << allTypes << epar << ';';
        }

        /*
         * Strong guarantee: commented-out code marked "Strong guarantee" generates
         * a copy-assignment operator that provides the strong exception guarantee.
         * For now, this is commented out, and we use the compiler-generated
         * copy-assignment operator. However, that one does not provide the strong
         * guarantee.

        H << ';';
        if(!p->isAbstract())
        {
            H << nl << name << "& operator=(const " << name << "&)";
            if(allDataMembers.empty())
            {
                H << " { return *this; }";
            }
            H << ';';
        }

        //
        // __swap() is static because classes may be abstract, so we
        // can't use a non-static member function when we do an upcall
        // from a non-abstract derived __swap to the __swap in an abstract base.
        //
        H << sp << nl << "static void __swap(" << name << "&, " << name << "&) throw()";
        if(allDataMembers.empty())
        {
            H << " {}";
        }
        H << ';';
        H << nl << "void swap(" << name << "& rhs) throw()";
        H << sb;
        if(!allDataMembers.empty())
        {
            H << nl << "__swap(*this, rhs);";
        }
        H << eb;

         * Strong guarantee
         */

        emitOneShotConstructor(p);

        /*
         * Strong guarantee

        if(!allDataMembers.empty())
        {
            C << sp << nl << "void";
            C << nl << scoped.substr(2) << "::__swap(" << name << "& __lhs, " << name << "& __rhs) throw()";
            C << sb;

            if(base)
            {
                emitUpcall(base, "::__swap(__lhs, __rhs);");
            }

            //
            // We use a map to remember for which types we have already declared
            // a temporary variable and reuse that variable if a class has
            // more than one member of the same type. That way, we don't use more
            // temporaries than necessary. (::std::swap() instantiates a new temporary
            // each time it is used.)
            //
            map<string, int> tmpMap;
            map<string, int>::iterator pos;
            int tmpCount = 0;

            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                string memberName = fixKwd((*q)->name());
                TypePtr type = (*q)->type();
                BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
                if(builtin && builtin->kind() != Builtin::KindString
                   || EnumPtr::dynamicCast(type) || ProxyPtr::dynamicCast(type)
                   || ClassDeclPtr::dynamicCast(type) || StructPtr::dynamicCast(type))
                {
                    //
                    // For built-in types (except string), enums, proxies, structs, and classes,
                    // do the swap via a temporary variable.
                    //
                    string typeName = typeToString(type);
                    pos = tmpMap.find(typeName);
                    if(pos == tmpMap.end())
                    {
                        pos = tmpMap.insert(pos, make_pair(typeName, tmpCount));
                        C << nl << typeName << " __tmp" << tmpCount << ';';
                        tmpCount++;
                    }
                    C << nl << "__tmp" << pos->second << " = __rhs." << memberName << ';';
                    C << nl << "__rhs." << memberName << " = __lhs." << memberName << ';';
                    C << nl << "__lhs." << memberName << " = __tmp" << pos->second << ';';
                }
                else
                {
                    //
                    // For dictionaries, vectors, and maps, use the standard container's
                    // swap() (which is usually optimized).
                    //
                    C << nl << "__lhs." << memberName << ".swap(__rhs." << memberName << ");";
                }
            }
            C << eb;

            if(!p->isAbstract())
            {
                C << sp << nl << scoped << "&";
                C << nl << scoped.substr(2) << "::operator=(const " << name << "& __rhs)";
                C << sb;
                C << nl << name << " __tmp(__rhs);";
                C << nl << "__swap(*this, __tmp);";
                C << nl << "return *this;";
                C << eb;
            }
        }

         * Strong guarantee
         */
    }

    if(!p->isLocal())
    {
        H << nl << "virtual ::Ice::ObjectPtr ice_clone() const;";

        C << sp;
        C << nl << "::Ice::ObjectPtr";
        C << nl << scoped.substr(2) << "::ice_clone() const";
        C << sb;
        if(!p->isAbstract())
        {
            C << nl << fixKwd(p->scope()) << p->name() << "Ptr __p = new " << scoped << "(*this);";
            C << nl << "return __p;";
        }
        else
        {
            C << nl << "throw ::Ice::CloneNotImplementedException(__FILE__, __LINE__);";
            C << nl << "return 0; // to avoid a warning with some compilers";
        }
        C << eb;

        ClassList allBases = p->allBases();
        StringList ids;
#if defined(__IBMCPP__) && defined(NDEBUG)
//
// VisualAge C++ 6.0 does not see that ClassDef is a Contained,
// when inlining is on. The code below issues a warning: better
// than an error!
//
        transform(allBases.begin(), allBases.end(), back_inserter(ids), ::IceUtil::constMemFun<string,ClassDef>(&Contained::scoped));
#else
        transform(allBases.begin(), allBases.end(), back_inserter(ids), ::IceUtil::constMemFun(&Contained::scoped));
#endif
        StringList other;
        other.push_back(p->scoped());
        other.push_back("::Ice::Object");
        other.sort();
        ids.merge(other);
        ids.unique();
        StringList::const_iterator firstIter = ids.begin();
        StringList::const_iterator scopedIter = find(ids.begin(), ids.end(), p->scoped());
        assert(scopedIter != ids.end());
        StringList::difference_type scopedPos = IceUtilInternal::distance(firstIter, scopedIter);

        H << sp;
        H << nl << "virtual bool ice_isA"
          << "(const ::std::string&, const ::Ice::Current& = ::Ice::Current()) const;";
        H << nl << "virtual ::std::vector< ::std::string> ice_ids"
          << "(const ::Ice::Current& = ::Ice::Current()) const;";
        H << nl << "virtual const ::std::string& ice_id(const ::Ice::Current& = ::Ice::Current()) const;";
        H << nl << "static const ::std::string& ice_staticId();";
        if(!dataMembers.empty())
        {
            H << sp;
        }

        string flatName = p->flattenedScope() + p->name() + "_ids";

        C << sp;
        C << nl << "static const ::std::string " << flatName << '[' << ids.size() << "] =";
        C << sb;

        StringList::const_iterator r = ids.begin();
        while(r != ids.end())
        {
            C << nl << '"' << *r << '"';
            if(++r != ids.end())
            {
                C << ',';
            }
        }
        C << eb << ';';

        C << sp;
        C << nl << "bool" << nl << scoped.substr(2)
          << "::ice_isA(const ::std::string& _s, const ::Ice::Current&) const";
        C << sb;
        C << nl << "return ::std::binary_search(" << flatName << ", " << flatName << " + " << ids.size() << ", _s);";
        C << eb;

        C << sp;
        C << nl << "::std::vector< ::std::string>" << nl << scoped.substr(2)
          << "::ice_ids(const ::Ice::Current&) const";
        C << sb;
        C << nl << "return ::std::vector< ::std::string>(&" << flatName << "[0], &" << flatName
          << '[' << ids.size() << "]);";
        C << eb;

        C << sp;
        C << nl << "const ::std::string&" << nl << scoped.substr(2)
          << "::ice_id(const ::Ice::Current&) const";
        C << sb;
        C << nl << "return " << flatName << '[' << scopedPos << "];";
        C << eb;

        C << sp;
        C << nl << "const ::std::string&" << nl << scoped.substr(2) << "::ice_staticId()";
        C << sb;
        C << nl << "return " << flatName << '[' << scopedPos << "];";
        C << eb;

        emitGCFunctions(p);
    }

    return true;
}

void
Slice::Gen::ObjectVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    string scoped = fixKwd(p->scoped());
    string scope = fixKwd(p->scope());

    if(!p->isLocal())
    {
        ClassList bases = p->bases();
        ClassDefPtr base;
        if(!bases.empty() && !bases.front()->isInterface())
        {
            base = bases.front();
        }

        OperationList allOps = p->allOperations();
        if(!allOps.empty())
        {
            StringList allOpNames;
#if defined(__IBMCPP__) && defined(NDEBUG)
//
// See comment for transform above
//
            transform(allOps.begin(), allOps.end(), back_inserter(allOpNames),
                      ::IceUtil::constMemFun<string,Operation>(&Contained::name));
#else
            transform(allOps.begin(), allOps.end(), back_inserter(allOpNames),
                      ::IceUtil::constMemFun(&Contained::name));
#endif
            allOpNames.push_back("ice_id");
            allOpNames.push_back("ice_ids");
            allOpNames.push_back("ice_isA");
            allOpNames.push_back("ice_ping");
            allOpNames.sort();
            allOpNames.unique();

            StringList::const_iterator q;

            H << sp;
            H << nl
              << "virtual ::Ice::DispatchStatus __dispatch(::IceInternal::Incoming&, const ::Ice::Current&);";

            string flatName = p->flattenedScope() + p->name() + "_all";
            C << sp;
            C << nl << "static ::std::string " << flatName << "[] =";
            C << sb;
            q = allOpNames.begin();
            while(q != allOpNames.end())
            {
                C << nl << '"' << *q << '"';
                if(++q != allOpNames.end())
                {
                    C << ',';
                }
            }
            C << eb << ';';
            C << sp;
            C << nl << "::Ice::DispatchStatus" << nl << scoped.substr(2)
              << "::__dispatch(::IceInternal::Incoming& in, const ::Ice::Current& current)";
            C << sb;

            C << nl << "::std::pair< ::std::string*, ::std::string*> r = "
              << "::std::equal_range(" << flatName << ", " << flatName << " + " << allOpNames.size()
              << ", current.operation);";
            C << nl << "if(r.first == r.second)";
            C << sb;
            C << nl << "throw ::Ice::OperationNotExistException(__FILE__, __LINE__, current.id, "
              << "current.facet, current.operation);";
            C << eb;
            C << sp;
            C << nl << "switch(r.first - " << flatName << ')';
            C << sb;
            int i = 0;
            for(q = allOpNames.begin(); q != allOpNames.end(); ++q)
            {
                C << nl << "case " << i++ << ':';
                C << sb;
                C << nl << "return ___" << *q << "(in, current);";
                C << eb;
            }
            C << eb;
            C << sp;
            C << nl << "assert(false);";
            C << nl << "throw ::Ice::OperationNotExistException(__FILE__, __LINE__, current.id, "
              << "current.facet, current.operation);";
            C << eb;


            //
            // Check if we need to generate ice_operationAttributes()
            //
            map<string, int> attributesMap;
            for(OperationList::iterator r = allOps.begin(); r != allOps.end(); ++r)
            {
                int attributes = (*r)->attributes();
                if(attributes != 0)
                {
                    attributesMap.insert(map<string, int>::value_type((*r)->name(), attributes));
                }
            }

            if(!attributesMap.empty())
            {
                H << sp;
                H << nl
                  << "virtual ::Ice::Int ice_operationAttributes(const ::std::string&) const;";

                string opAttrFlatName = p->flattenedScope() + p->name() + "_operationAttributes";

                C << sp;
                C << nl << "static int " << opAttrFlatName << "[] = ";
                C << sb;

                q = allOpNames.begin();
                while(q != allOpNames.end())
                {
                    int attributes = 0;
                    string opName = *q;
                    map<string, int>::iterator it = attributesMap.find(opName);
                    if(it != attributesMap.end())
                    {
                        attributes = it->second;
                    }
                    C << nl << attributes;

                    if(++q != allOpNames.end())
                    {
                        C << ',';
                    }
                    C << " // " << opName;
                }

                C << eb << ';';
                C << sp;

                C << nl << "::Ice::Int" << nl << scoped.substr(2)
                  << "::ice_operationAttributes(const ::std::string& opName) const";
                C << sb;

                C << nl << "::std::pair< ::std::string*, ::std::string*> r = "
                  << "::std::equal_range(" << flatName << ", " << flatName << " + " << allOpNames.size()
                  << ", opName);";
                C << nl << "if(r.first == r.second)";
                C << sb;
                C << nl << "return -1;";
                C << eb;

                C << nl << "return " << opAttrFlatName << "[r.first - " << flatName << "];";
                C << eb;
            }
        }

        H << sp;
        H << nl << "virtual void __write(::IceInternal::BasicStream*) const;";
        H << nl << "virtual void __read(::IceInternal::BasicStream*, bool);";
      
        
        H.zeroIndent();
        H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
        H.restoreIndent();
        H << nl << "//Stream api is not supported in VC++ 6";
        H.zeroIndent();
        H << nl << "#else";
        H.restoreIndent();
        H << nl << "virtual void __write(const ::Ice::OutputStreamPtr&) const;";
        H << nl << "virtual void __read(const ::Ice::InputStreamPtr&, bool);";
        H.zeroIndent();
        H << nl << "#endif";
        H.restoreIndent();
        
        C << sp;
        C << nl << "void" << nl << scoped.substr(2)
          << "::__write(::IceInternal::BasicStream* __os) const";
        C << sb;
        C << nl << "__os->writeTypeId(ice_staticId());";
        C << nl << "__os->startWriteSlice();";
        DataMemberList dataMembers = p->dataMembers();
        DataMemberList::const_iterator q;
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            writeMarshalUnmarshalCode(C, (*q)->type(), fixKwd((*q)->name()), true, "", true, (*q)->getMetaData());
        }
        C << nl << "__os->endWriteSlice();";
        emitUpcall(base, "::__write(__os);");
        C << eb;
        C << sp;
        C << nl << "void" << nl << scoped.substr(2) << "::__read(::IceInternal::BasicStream* __is, bool __rid)";
        C << sb;
        C << nl << "if(__rid)";
        C << sb;
        C << nl << "::std::string myId;";
        C << nl << "__is->readTypeId(myId);";
        C << eb;
        C << nl << "__is->startReadSlice();";
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            writeMarshalUnmarshalCode(C, (*q)->type(), fixKwd((*q)->name()), false, "", true, (*q)->getMetaData());
        }
        C << nl << "__is->endReadSlice();";
        emitUpcall(base, "::__read(__is, true);");
        C << eb;

        if(_stream)
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp;
            C << nl << "void" << nl << scoped.substr(2) << "::__write(const ::Ice::OutputStreamPtr& __outS) const";
            C << sb;
            C << nl << "__outS->writeTypeId(ice_staticId());";
            C << nl << "__outS->startSlice();";
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                writeStreamMarshalUnmarshalCode(C, (*q)->type(), (*q)->name(), true, "", (*q)->getMetaData(), 
                                                _useWstring);
            }
            C << nl << "__outS->endSlice();";
            emitUpcall(base, "::__write(__outS);");
            C << eb;
            C << sp;
            C << nl << "void" << nl << scoped.substr(2) << "::__read(const ::Ice::InputStreamPtr& __inS, bool __rid)";
            C << sb;
            C << nl << "if(__rid)";
            C << sb;
            C << nl << "__inS->readTypeId();";
            C << eb;
            C << nl << "__inS->startSlice();";
            for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
            {
                writeStreamMarshalUnmarshalCode(C, (*q)->type(), (*q)->name(), false, "", (*q)->getMetaData(),
                                                _useWstring);
            }
            C << nl << "__inS->endSlice();";
            emitUpcall(base, "::__read(__inS, true);");
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
        else
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            //
            // Emit placeholder functions to catch errors.
            //
            C << sp;
            C << nl << "void" << nl << scoped.substr(2) << "::__write(const ::Ice::OutputStreamPtr&) const";
            C << sb;
            C << nl << "Ice::MarshalException ex(__FILE__, __LINE__);";
            C << nl << "ex.reason = \"type " << scoped.substr(2) << " was not generated with stream support\";";
            C << nl << "throw ex;";
            C << eb;
            C << sp;
            C << nl << "void" << nl << scoped.substr(2) << "::__read(const ::Ice::InputStreamPtr&, bool)";
            C << sb;
            C << nl << "Ice::MarshalException ex(__FILE__, __LINE__);";
            C << nl << "ex.reason = \"type " << scoped.substr(2) << " was not generated with stream support\";";
            C << nl << "throw ex;";
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }

        if(!p->isAbstract())
        {
            H << sp << nl << "static const ::Ice::ObjectFactoryPtr& ice_factory();";

            string factoryName = "__F" + p->flattenedScope() + p->name();
            C << sp;
            C << nl << "class " << factoryName << " : public ::Ice::ObjectFactory";
            C << sb;
            C.dec();
            C << nl << "public:";
            C.inc();
            C << sp << nl << "virtual ::Ice::ObjectPtr" << nl << "create(const ::std::string& type)";
            C << sb;
            C << nl << "assert(type == " << scoped << "::ice_staticId());";
            C << nl << "return new " << scoped << ';';
            C << eb;
            C << sp << nl << "virtual void" << nl << "destroy()";
            C << sb;
            C << eb;
            C << eb << ';';

            string flatName = factoryName + "_Ptr";
            C << sp;
            C << nl << "static ::Ice::ObjectFactoryPtr " << flatName << " = new " << factoryName << ';';

            C << sp << nl << "const ::Ice::ObjectFactoryPtr&" << nl << scoped.substr(2) << "::ice_factory()";
            C << sb;
            C << nl << "return " << flatName << ';';
            C << eb;

            C << sp;
            C << nl << "class " << factoryName << "__Init";
            C << sb;
            C.dec();
            C << nl << "public:";
            C.inc();
            C << sp << nl << factoryName << "__Init()";
            C << sb;
            C << nl << "::IceInternal::factoryTable->addObjectFactory(" << scoped << "::ice_staticId(), "
              << scoped << "::ice_factory());";
            C << eb;
            C << sp << nl << "~" << factoryName << "__Init()";
            C << sb;
            C << nl << "::IceInternal::factoryTable->removeObjectFactory(" << scoped << "::ice_staticId());";
            C << eb;
            C << eb << ';';

            C << sp;
            C << nl << "static " << factoryName << "__Init " << factoryName << "__i;";
            C << sp << nl << "#ifdef __APPLE__";
            std::string initfuncname = "__F" + p->flattenedScope() + p->name() + "__initializer";
            C << nl << "extern \"C\" { void " << initfuncname << "() {} }";
            C << nl << "#endif";
        }
    }

    bool inProtected = false;

    if(!p->isAbstract())
    {
        //
        // We add a protected destructor to force heap instantiation of the class.
        //
        H.dec();
        H << sp << nl << "protected:";
        H.inc();
        H << sp << nl << "virtual ~" << fixKwd(p->name()) << "() {}";

        if(!_doneStaticSymbol)
        {
            H << sp << nl << "friend class " << p->name() << "__staticInit;";
        }

        inProtected = true;
    }

    //
    // Emit data members. Access visibility may be specified by metadata.
    //
    DataMemberList dataMembers = p->dataMembers();
    DataMemberList::const_iterator q;
    bool prot = p->hasMetaData("protected");
    for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
    {
        TypePtr type = (*q)->type();
        if(SequencePtr::dynamicCast(type))
        {
            SequencePtr s = SequencePtr::dynamicCast(type);
            BuiltinPtr builtin = BuiltinPtr::dynamicCast(s->type());
            if(builtin && builtin->kind() == Builtin::KindByte)
            {
                StringList metaData = s->getMetaData();
                bool protobuf;
                findMetaData(s, metaData, protobuf);
                if(protobuf)
                {
                    emitWarning((*q)->file(), (*q)->line(), "protobuf cannot be used as a class member in C++");
                }
            }
        }

        if(prot || (*q)->hasMetaData("protected"))
        {
            if(!inProtected)
            {
                H.dec();
                H << sp << nl << "protected:";
                H.inc();
                inProtected = true;
            }
        }
        else
        {
            if(inProtected)
            {
                H.dec();
                H << sp << nl << "public:";
                H.inc();
                inProtected = false;
            }
        }

        string name = fixKwd((*q)->name());
        string s = typeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
        H << sp << nl << s << ' ' << name << ';';
    }

    H << eb << ';';

    if(!p->isAbstract() && !_doneStaticSymbol)
    {
        //
        // We need an instance here to trigger initialization if the implementation is in a shared library.
        // But we do this only once per source file, because a single instance is sufficient to initialize
        // all of the globals in a shared library.
        //
        // For a Slice class Foo, we instantiate a dummy class Foo__staticInit instead of using a static
        // Foo instance directly because Foo has a protected destructor.
        //
        H << sp << nl << "class " << p->name() << "__staticInit";
        H << sb;
        H.dec();
        H << nl << "public:";
        H.inc();
        H << sp << nl << scoped << " _init;";
        H << eb << ';';
        _doneStaticSymbol = true;
        H << sp << nl << "static " << p->name() << "__staticInit _" << p->name() << "_init;";
    }

    if(p->isLocal())
    {
        H << sp;
        H << nl << "inline bool operator==(const " << fixKwd(p->name()) << "& l, const " << fixKwd(p->name()) << "& r)";
        H << sb;
        H << nl << "return static_cast<const ::Ice::LocalObject&>(l) == static_cast<const ::Ice::LocalObject&>(r);";
        H << eb;
        H << sp;
        H << nl << "inline bool operator<(const " << fixKwd(p->name()) << "& l, const " << fixKwd(p->name()) << "& r)";
        H << sb;
        H << nl << "return static_cast<const ::Ice::LocalObject&>(l) < static_cast<const ::Ice::LocalObject&>(r);";
        H << eb;
    }
    else
    {
        C << sp << nl << "void "
	  << (_dllExport.empty() ? "" : "ICE_DECLSPEC_EXPORT ");
        C << nl << scope.substr(2) << "__patch__" << p->name() << "Ptr(void* __addr, ::Ice::ObjectPtr& v)";
        C << sb;
        C << nl << scope << p->name() << "Ptr* p = static_cast< " << scope << p->name() << "Ptr*>(__addr);";
        C << nl << "assert(p);";
        C << nl << "*p = " << scope << p->name() << "Ptr::dynamicCast(v);";
        C << nl << "if(v && !*p)";
        C << sb;
        C << nl << "IceInternal::Ex::throwUOE(" << scoped << "::ice_staticId(), v->ice_id());";
        C << eb;
        C << eb;

        H << sp;
        H << nl << "inline bool operator==(const " << fixKwd(p->name()) << "& l, const " << fixKwd(p->name()) << "& r)";
        H << sb;
        H << nl << "return static_cast<const ::Ice::Object&>(l) == static_cast<const ::Ice::Object&>(r);";
        H << eb;
        H << sp;
        H << nl << "inline bool operator<(const " << fixKwd(p->name()) << "& l, const " << fixKwd(p->name()) << "& r)";
        H << sb;
        H << nl << "return static_cast<const ::Ice::Object&>(l) < static_cast<const ::Ice::Object&>(r);";
        H << eb;
    }

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::ObjectVisitor::visitExceptionStart(const ExceptionPtr&)
{
    return false;
}

bool
Slice::Gen::ObjectVisitor::visitStructStart(const StructPtr&)
{
    return false;
}

void
Slice::Gen::ObjectVisitor::visitOperation(const OperationPtr& p)
{
    string name = p->name();
    string scoped = fixKwd(p->scoped());
    string scope = fixKwd(p->scope());

    TypePtr ret = p->returnType();
    string retS = returnTypeToString(ret, p->getMetaData(), _useWstring);

    string params = "(";
    string paramsDecl = "(";
    string args = "(";

    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);
    string classNameAMD = "AMD_" + cl->name();
    string classScope = fixKwd(cl->scope());
    string classScopedAMD = classScope + classNameAMD;

    string paramsAMD = "(const " + classScopedAMD + '_' + name + "Ptr&, ";
    string paramsDeclAMD = "(const " + classScopedAMD + '_' + name + "Ptr& __cb, ";
    string argsAMD = "(__cb, ";

    ParamDeclList inParams;
    ParamDeclList outParams;
    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        string paramName = fixKwd((*q)->name());
        TypePtr type = (*q)->type();
        bool isOutParam = (*q)->isOutParam();
        string typeString;
        if(isOutParam)
        {
            outParams.push_back(*q);
            typeString = outputTypeToString(type, (*q)->getMetaData(), _useWstring);
        }
        else
        {
            inParams.push_back(*q);
            typeString = inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
        }

        if(q != paramList.begin())
        {
            params += ", ";
            paramsDecl += ", ";
            args += ", ";
        }

        params += typeString;
        paramsDecl += typeString;
        paramsDecl += ' ';
        paramsDecl += paramName;
        args += paramName;

        if(!isOutParam)
        {
            paramsAMD += typeString;
            paramsAMD += ", ";
            paramsDeclAMD += typeString;
            paramsDeclAMD += ' ';
            paramsDeclAMD += paramName;
            paramsDeclAMD += ", ";
            argsAMD += paramName;
            argsAMD += ", ";
        }
    }

    if(!cl->isLocal())
    {
        if(!paramList.empty())
        {
            params += ", ";
            paramsDecl += ", ";
            args += ", ";
        }

        params += "const ::Ice::Current& = ::Ice::Current())";
        paramsDecl += "const ::Ice::Current& __current)";
        args += "__current)";
    }
    else
    {
        params += ')';
        paramsDecl += ')';
        args += ')';
    }

    paramsAMD += "const ::Ice::Current& = ::Ice::Current())";
    paramsDeclAMD += "const ::Ice::Current& __current)";
    argsAMD += "__current)";

    bool isConst = (p->mode() == Operation::Nonmutating) || p->hasMetaData("cpp:const");
    bool amd = !cl->isLocal() && (cl->hasMetaData("amd") || p->hasMetaData("amd"));

    string deprecateSymbol = getDeprecateSymbol(p, cl);

    H << sp;
    if(!amd)
    {
        H << nl << deprecateSymbol << "virtual " << retS << ' ' << fixKwd(name) << params
          << (isConst ? " const" : "") << " = 0;";
    }
    else
    {
        H << nl << deprecateSymbol << "virtual void " << name << "_async" << paramsAMD
          << (isConst ? " const" : "") << " = 0;";
    }

    if(!cl->isLocal())
    {
        H << nl << "::Ice::DispatchStatus ___" << name
          << "(::IceInternal::Incoming&, const ::Ice::Current&)" << (isConst ? " const" : "") << ';';

        C << sp;
        C << nl << "::Ice::DispatchStatus" << nl << scope.substr(2) << "___" << name
          << "(::IceInternal::Incoming& __inS";
        C << ", const ::Ice::Current& __current)" << (isConst ? " const" : "");
        C << sb;
        if(!amd)
        {
            ExceptionList throws = p->throws();
            throws.sort();
            throws.unique();

            //
            // Arrange exceptions into most-derived to least-derived order. If we don't
            // do this, a base exception handler can appear before a derived exception
            // handler, causing compiler warnings and resulting in the base exception
            // being marshaled instead of the derived exception.
            //

#if defined(__SUNPRO_CC)
            throws.sort(derivedToBaseCompare);
#else
            throws.sort(Slice::DerivedToBaseCompare());
#endif

            C << nl << "__checkMode(" << operationModeToString(p->mode()) << ", __current.mode);";

            if(!inParams.empty())
            {
                C << nl << "::IceInternal::BasicStream* __is = __inS.is();";
                C << nl << "__is->startReadEncaps();";
                writeAllocateCode(C, inParams, 0, StringList(), _useWstring | TypeContextInParam);
                writeUnmarshalCode(C, inParams, 0, StringList(), TypeContextInParam);
                if(p->sendsClasses())
                {
                    C << nl << "__is->readPendingObjects();";
                }
                C << nl << "__is->endReadEncaps();";
            }
            else
            {
                C << nl << "__inS.is()->skipEmptyEncaps();";
            }

            if(ret || !outParams.empty() || !throws.empty())
            {
                C << nl << "::IceInternal::BasicStream* __os = __inS.os();";
            }
            writeAllocateCode(C, outParams, 0, StringList(), _useWstring);
            if(!throws.empty())
            {
                C << nl << "try";
                C << sb;
            }
            C << nl;
            if(ret)
            {
                C << retS << " __ret = ";
            }
            C << fixKwd(name) << args << ';';
            writeMarshalCode(C, outParams, ret, p->getMetaData());
            if(p->returnsClasses())
            {
                C << nl << "__os->writePendingObjects();";
            }
            if(!throws.empty())
            {
                C << eb;
                ExceptionList::const_iterator r;
                for(r = throws.begin(); r != throws.end(); ++r)
                {
                    C << nl << "catch(const " << fixKwd((*r)->scoped()) << "& __ex)";
                    C << sb;
                    C << nl << "__os->write(__ex);";
                    C << nl << "return ::Ice::DispatchUserException;";
                    C << eb;
                }
            }

            C << nl << "return ::Ice::DispatchOK;";
        }
        else
        {
            C << nl << "__checkMode(" << operationModeToString(p->mode()) << ", __current.mode);";

            if(!inParams.empty())
            {
                C << nl << "::IceInternal::BasicStream* __is = __inS.is();";
                C << nl << "__is->startReadEncaps();";
                writeAllocateCode(C, inParams, 0, StringList(), _useWstring | TypeContextInParam);
                writeUnmarshalCode(C, inParams, 0, StringList(), TypeContextInParam);
                if(p->sendsClasses())
                {
                    C << nl << "__is->readPendingObjects();";
                }
                C << nl << "__is->endReadEncaps();";
            }
            else
            {
                C << nl << "__inS.is()->skipEmptyEncaps();";
            }

            C << nl << classScopedAMD << '_' << name << "Ptr __cb = new IceAsync" << classScopedAMD << '_' << name
              << "(__inS);";
            C << nl << "try";
            C << sb;
            C << nl << name << "_async" << argsAMD << ';';
            C << eb;
            C << nl << "catch(const ::std::exception& __ex)";
            C << sb;
            C << nl << "__cb->ice_exception(__ex);";
            C << eb;
            C << nl << "catch(...)";
            C << sb;
            C << nl << "__cb->ice_exception();";
            C << eb;
            C << nl << "return ::Ice::DispatchAsync;";
        }
        C << eb;
    }
}

void
Slice::Gen::ObjectVisitor::emitGCFunctions(const ClassDefPtr& p)
{
    string scoped = fixKwd(p->scoped());
    ClassList bases = p->bases();
    DataMemberList dataMembers = p->dataMembers();

    //
    // A class can potentially be part of a cycle if it (recursively) contains class
    // members. If so, we override __incRef(), __decRef(), and __addObject() and, hence, consider
    // instances of the class as candidates for collection by the garbage collector.
    // We override __incRef(), __decRef(), and __addObject() only once, in the basemost potentially
    // cyclic class in an inheritance hierarchy.
    //
    bool hasBaseClass = !bases.empty() && !bases.front()->isInterface();
    bool canBeCyclic = p->canBeCyclic();
    bool override = canBeCyclic && (!hasBaseClass || !bases.front()->canBeCyclic());

    if(override)
    {
        H << nl << "virtual void __incRef();";

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__incRef()";
        C << sb;
        C << nl << "__gcIncRef();";
        C << eb;

        H << nl << "virtual void __decRef();";

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__decRef()";
        C << sb;
        C << nl << "__gcDecRef();";
        C << eb;

        H << nl << "virtual void __addObject(::IceInternal::GCCountMap&);";

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__addObject(::IceInternal::GCCountMap& _c)";
        C << sb;
        C << nl << "::IceInternal::GCCountMap::iterator pos = _c.find(this);";
        C << nl << "if(pos == _c.end())";
        C << sb;
        C << nl << "_c[this] = 1;";
        C << eb;
        C << nl << "else";
        C << sb;
        C << nl << "++pos->second;";
        C << eb;
        C << eb;

        H << nl << "virtual bool __usesClasses();";

        C << sp << nl << "bool" << nl << scoped.substr(2) << "::__usesClasses()";
        C << sb;
        C << nl << "return true;";
        C << eb;
    }

    //
    // __gcReachable() and __gcClear() are overridden by the basemost class that
    // can be cyclic, plus all classes derived from that class.
    //
    if(canBeCyclic)
    {
        H << nl << "virtual void __gcReachable(::IceInternal::GCCountMap&) const;";

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__gcReachable(::IceInternal::GCCountMap& _c) const";
        C << sb;

        bool hasCyclicBase = hasBaseClass && bases.front()->canBeCyclic();
        if(hasCyclicBase)
        {
            emitUpcall(bases.front(), "::__gcReachable(_c);");
        }
        for(DataMemberList::const_iterator i = dataMembers.begin(); i != dataMembers.end(); ++i)
        {
            if((*i)->type()->usesClasses())
            {
                emitGCInsertCode((*i)->type(), fixKwd((*i)->name()), "", 0);
            }
        }
        C << eb;

        H << nl << "virtual void __gcClear();";

        C << sp << nl << "void" << nl << scoped.substr(2) << "::__gcClear()";
        C << sb;
        if(hasCyclicBase)
        {
            emitUpcall(bases.front(), "::__gcClear();");
        }
        for(DataMemberList::const_iterator j = dataMembers.begin(); j != dataMembers.end(); ++j)
        {
            if((*j)->type()->usesClasses())
            {
                emitGCClearCode((*j)->type(), fixKwd((*j)->name()), "", 0);
            }
        }
        C << eb;
    }
}

void
Slice::Gen::ObjectVisitor::emitGCInsertCode(const TypePtr& p, const string& prefix, const string& name, int level)
{
    if((BuiltinPtr::dynamicCast(p) && BuiltinPtr::dynamicCast(p)->kind() == Builtin::KindObject)
       || ClassDeclPtr::dynamicCast(p))
    {
        C << nl << "if(" << prefix << name << ')';
        C << sb;
        ClassDeclPtr decl = ClassDeclPtr::dynamicCast(p);
        if(decl)
        {
            C << nl << "::IceInternal::upCast(" << prefix << name << ".get())->__addObject(_c);";
        }
        else
        {
            C << nl << prefix << name << "->__addObject(_c);";
        }
        C << eb;
    }
    else if(StructPtr::dynamicCast(p))
    {
        StructPtr s = StructPtr::dynamicCast(p);
        DataMemberList dml = s->dataMembers();
        for(DataMemberList::const_iterator i = dml.begin(); i != dml.end(); ++i)
        {
            if((*i)->type()->usesClasses())
            {
                emitGCInsertCode((*i)->type(), prefix + name + ".", fixKwd((*i)->name()), ++level);
            }
        }
    }
    else if(DictionaryPtr::dynamicCast(p))
    {
        DictionaryPtr d = DictionaryPtr::dynamicCast(p);
        string scoped = fixKwd(d->scoped());
        ostringstream tmp;
        tmp << "_i" << level;
        string iterName = tmp.str();
        C << sb;
        C << nl << "for(" << scoped << "::const_iterator " << iterName << " = " << prefix + name
          << ".begin(); " << iterName << " != " << prefix + name << ".end(); ++" << iterName << ")";
        C << sb;
        emitGCInsertCode(d->valueType(), "", string("(*") + iterName + ").second", ++level);
        C << eb;
        C << eb;
    }
    else if(SequencePtr::dynamicCast(p))
    {
        SequencePtr s = SequencePtr::dynamicCast(p);
        string scoped = fixKwd(s->scoped());
        ostringstream tmp;
        tmp << "_i" << level;
        string iterName = tmp.str();
        C << sb;
        C << nl << "for(" << scoped << "::const_iterator " << iterName << " = " << prefix + name
          << ".begin(); " << iterName << " != " << prefix + name << ".end(); ++" << iterName << ")";
        C << sb;
        emitGCInsertCode(s->type(), string("(*") + iterName + ")", "", ++level);
        C << eb;
        C << eb;
    }
}

void
Slice::Gen::ObjectVisitor::emitGCClearCode(const TypePtr& p, const string& prefix, const string& name, int level)
{
    if((BuiltinPtr::dynamicCast(p) && BuiltinPtr::dynamicCast(p)->kind() == Builtin::KindObject)
       || ClassDeclPtr::dynamicCast(p))
    {
        C << nl << "if(" << prefix << name << ")";
        C << sb;
        ClassDeclPtr decl = ClassDeclPtr::dynamicCast(p);
        if(decl)
        {
            C << nl << "if(" << "::IceInternal::upCast(" << prefix << name << ".get())->__usesClasses())";
            C << sb;
            C << nl << "::IceInternal::upCast(" << prefix << name << ".get())->__decRefUnsafe();";
            C << nl << prefix << name << ".__clearHandleUnsafe();";

        }
        else
        {
            C << nl << "if(" << prefix << name << "->__usesClasses())";
            C << sb;
            C << nl << prefix << name << "->__decRefUnsafe();";
            C << nl << prefix << name << ".__clearHandleUnsafe();";
        }
        C << eb;
        C << nl << "else";
        C << sb;
        C << nl << prefix << name << " = 0;";
        C << eb;
        C << eb;
    }
    else if(StructPtr::dynamicCast(p))
    {
        StructPtr s = StructPtr::dynamicCast(p);
        DataMemberList dml = s->dataMembers();
        for(DataMemberList::const_iterator i = dml.begin(); i != dml.end(); ++i)
        {
            if((*i)->type()->usesClasses())
            {
                emitGCClearCode((*i)->type(), prefix + name + ".", fixKwd((*i)->name()), ++level);
            }
        }
    }
    else if(DictionaryPtr::dynamicCast(p))
    {
        DictionaryPtr d = DictionaryPtr::dynamicCast(p);
        string scoped = fixKwd(d->scoped());
        ostringstream tmp;
        tmp << "_i" << level;
        string iterName = tmp.str();
        C << sb;
        C << nl << "for(" << scoped << "::iterator " << iterName << " = " << prefix + name
          << ".begin(); " << iterName << " != " << prefix + name << ".end(); ++" << iterName << ")";
        C << sb;
        emitGCClearCode(d->valueType(), "", string("(*") + iterName + ").second", ++level);
        C << eb;
        C << eb;
    }
    else if(SequencePtr::dynamicCast(p))
    {
        SequencePtr s = SequencePtr::dynamicCast(p);
        string scoped = fixKwd(s->scoped());
        ostringstream tmp;
        tmp << "_i" << level;
        string iterName = tmp.str();
        C << sb;
        C << nl << "for(" << scoped << "::iterator " << iterName << " = " << prefix + name
          << ".begin(); " << iterName << " != " << prefix + name << ".end(); ++" << iterName << ")";
        C << sb;
        emitGCClearCode(s->type(), "", string("(*") + iterName + ")", ++level);
        C << eb;
        C << eb;;
    }
}

bool
Slice::Gen::ObjectVisitor::emitVirtualBaseInitializers(const ClassDefPtr& p, bool virtualInheritance)
{
    DataMemberList allDataMembers = p->allDataMembers();
    if(allDataMembers.empty())
    {
        return false;
    }

    if(virtualInheritance)
    {
        ClassList bases = p->bases();
        if(!bases.empty() && !bases.front()->isInterface())
        {
            if(emitVirtualBaseInitializers(bases.front(), virtualInheritance))
            {
                C << ',';
            }
        }
    }

    string upcall = "(";
    DataMemberList::const_iterator q;
    for(q = allDataMembers.begin(); q != allDataMembers.end(); ++q)
    {
        if(q != allDataMembers.begin())
        {
            upcall += ", ";
        }
        upcall += "__ice_" + (*q)->name();
    }
    upcall += ")";

    C.zeroIndent();
    C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug";
    C.restoreIndent();
    C << nl << fixKwd(p->name()) << upcall;
    C.zeroIndent();
    C << nl << "#else";
    C.restoreIndent();
    C << nl << fixKwd(p->scoped()) << upcall;
    C.zeroIndent();
    C << nl << "#endif";
    C << nl;
    C.restoreIndent();

    return true;
}

void
Slice::Gen::ObjectVisitor::emitOneShotConstructor(const ClassDefPtr& p)
{
    DataMemberList allDataMembers = p->allDataMembers();
    DataMemberList::const_iterator q;

    vector<string> allParamDecls;

    for(q = allDataMembers.begin(); q != allDataMembers.end(); ++q)
    {
        string typeName = inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
        allParamDecls.push_back(typeName + " __ice_" + (*q)->name());
    }

    if(!allDataMembers.empty())
    {
        C << sp << nl << fixKwd(p->scoped()).substr(2) << "::" << fixKwd(p->name())
	  << spar << allParamDecls << epar << " :";
        C.inc();

        DataMemberList dataMembers = p->dataMembers();

        ClassList bases = p->bases();
        ClassDefPtr base;
        if(!bases.empty() && !bases.front()->isInterface())
        {
            if(emitVirtualBaseInitializers(bases.front(), p->hasMetaData("cpp:virtual")))
            {
                if(!dataMembers.empty())
                {
                    C << ',';
                }
            }
        }

        if(!dataMembers.empty())
        {
            C << nl;
        }
        for(q = dataMembers.begin(); q != dataMembers.end(); ++q)
        {
            if(q != dataMembers.begin())
            {
                C << ',' << nl;
            }
            string memberName = fixKwd((*q)->name());
            C << memberName << '(' << "__ice_" << (*q)->name() << ')';
        }

        C.dec();
        C << sb;
        C << eb;
    }
}

void
Slice::Gen::ObjectVisitor::emitUpcall(const ClassDefPtr& base, const string& call)
{
    C.zeroIndent();
    C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    C.restoreIndent();
    C << nl << (base ? fixKwd(base->name()) : string("Object")) << call;
    C.zeroIndent();
    C << nl << "#else";
    C.restoreIndent();
    C << nl << (base ? fixKwd(base->scoped()) : string("::Ice::Object")) << call;
    C.zeroIndent();
    C << nl << "#endif";
    C.restoreIndent();
}

Slice::Gen::AsyncCallbackVisitor::AsyncCallbackVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::AsyncCallbackVisitor::visitUnitStart(const UnitPtr& p)
{
    return p->hasNonLocalClassDefs();
}

void
Slice::Gen::AsyncCallbackVisitor::visitUnitEnd(const UnitPtr&)
{
}

bool
Slice::Gen::AsyncCallbackVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    H << sp << nl << "namespace " << fixKwd(p->name())  << nl << '{';

    return true;
}

void
Slice::Gen::AsyncCallbackVisitor::visitModuleEnd(const ModulePtr& p)
{
    _useWstring = resetUseWstring(_useWstringHist);

    H << sp << nl << '}';
}

bool
Slice::Gen::AsyncCallbackVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);
    return true;
}

void
Slice::Gen::AsyncCallbackVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::AsyncCallbackVisitor::visitOperation(const OperationPtr& p)
{
    ClassDefPtr cl = ClassDefPtr::dynamicCast(p->container());

    if(cl->isLocal() || cl->operations().empty())
    {
        return;
    }

    //
    // Write the callback base class and callback smart pointer.
    //
    string delName = "Callback_" + cl->name() + "_" + p->name();
    H << sp << nl << "class " << delName << "_Base : virtual public ::IceInternal::CallbackBase { };";
    H << nl << "typedef ::IceUtil::Handle< " << delName << "_Base> " << delName << "Ptr;";
}

Slice::Gen::AsyncCallbackTemplateVisitor::AsyncCallbackTemplateVisitor(Output& h, 
                                                                       Output& c,
                                                                       const string& dllExport)
    : H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::AsyncCallbackTemplateVisitor::visitUnitStart(const UnitPtr& p)
{
    return p->hasNonLocalClassDefs();
}

void
Slice::Gen::AsyncCallbackTemplateVisitor::visitUnitEnd(const UnitPtr&)
{
}

bool
Slice::Gen::AsyncCallbackTemplateVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    H << sp << nl << "namespace " << fixKwd(p->name())  << nl << '{';
    return true;
}

void
Slice::Gen::AsyncCallbackTemplateVisitor::visitModuleEnd(const ModulePtr& p)
{
    _useWstring = resetUseWstring(_useWstringHist);

    H << sp << nl << '}';
}

bool
Slice::Gen::AsyncCallbackTemplateVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);
    return true;
}

void
Slice::Gen::AsyncCallbackTemplateVisitor::visitClassDefEnd(const ClassDefPtr& p)
{
    _useWstring = resetUseWstring(_useWstringHist);
}

namespace
{

bool
usePrivateEnd(const OperationPtr& p)
{
    TypePtr ret = p->returnType();
    string retSEnd = returnTypeToString(ret, p->getMetaData(), TypeContextAMIEnd);
    string retSPrivateEnd = returnTypeToString(ret, p->getMetaData(), TypeContextAMIPrivateEnd);
        
    ParamDeclList outParams;
    vector<string> outDeclsEnd;
    vector<string> outDeclsPrivateEnd;

    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        if((*q)->isOutParam())
        {
            outDeclsEnd.push_back(outputTypeToString((*q)->type(), (*q)->getMetaData(), TypeContextAMIEnd));
            outDeclsPrivateEnd.push_back(outputTypeToString((*q)->type(), (*q)->getMetaData(), TypeContextAMIPrivateEnd));
        }
    }
    
    return retSEnd != retSPrivateEnd || outDeclsEnd != outDeclsPrivateEnd;
}

}

void
Slice::Gen::AsyncCallbackTemplateVisitor::visitOperation(const OperationPtr& p)
{
    generateOperation(p, false);
    generateOperation(p, true);
}

void
Slice::Gen::AsyncCallbackTemplateVisitor::generateOperation(const OperationPtr& p, bool withCookie)
{
    ClassDefPtr cl = ClassDefPtr::dynamicCast(p->container());
    if(cl->isLocal() || cl->operations().empty())
    {
        return;
    }

    string clName = cl->name();
    string clScope = fixKwd(cl->scope());
    string delName = "Callback_" + clName + "_" + p->name();
    string delTmplName = (withCookie ? "Callback_" : "CallbackNC_") + clName + "_" + p->name();

    TypePtr ret = p->returnType();
    string retS = inputTypeToString(ret, p->getMetaData(), _useWstring);
    string retEndArg = getEndArg(ret, p->getMetaData(), "__ret");
        
    ParamDeclList outParams;
    vector<string> outArgs;
    vector<string> outDecls;
    vector<string> outDeclsEnd;
    vector<string> outEndArgs;
        
    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        if((*q)->isOutParam())
        {
            outParams.push_back(*q);
            outArgs.push_back(fixKwd((*q)->name()));
            outEndArgs.push_back(getEndArg((*q)->type(), (*q)->getMetaData(), outArgs.back()));
            outDecls.push_back(inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring));
        }
    }

    string baseD;
    string inheritD;
    if(withCookie)
    {
        baseD = "::IceInternal::Callback<T, CT>";
        H << sp << nl << "template<class T, typename CT>";
        inheritD = p->returnsData() ? "::IceInternal::TwowayCallback<T, CT>" : "::IceInternal::OnewayCallback<T, CT>";
    }
    else
    {
        baseD = "::IceInternal::CallbackNC<T>";
        H << sp << nl << "template<class T>";
        inheritD = p->returnsData() ? "::IceInternal::TwowayCallbackNC<T>" : "::IceInternal::OnewayCallbackNC<T>";
    }

    H << nl << "class " << delTmplName << " : public " << delName << "_Base, public " << inheritD;
    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    H << sp << nl << "typedef IceUtil::Handle<T> TPtr;";

    string cookieT;
    string comCookieT;
    if(withCookie)
    {
        cookieT = "const CT&";
        comCookieT = " , const CT&";
    }

    H << sp << nl << "typedef void (T::*Exception)(const ::Ice::Exception&" << comCookieT << ");";
    H << nl << "typedef void (T::*Sent)(bool" << comCookieT << ");";
    if(p->returnsData())
    {
        //
        // typedefs for callbacks.
        //
        H << nl << "typedef void (T::*Response)" << spar;
        if(ret)
        {
            H << retS;
        }
        H << outDecls;
        if(withCookie)
        {
            H << cookieT;
        }
        H << epar << ';';
    }
    else
    {
        H << nl << "typedef void (T::*Response)(" << cookieT << ");";
    }

    //
    // constructor.
    //
    H << sp;
    H << nl << delTmplName << spar << "const TPtr& obj";
    H << "Response cb";
    H << "Exception excb";
    H << "Sent sentcb" << epar;
    H.inc();
    if(p->returnsData())
    {
        H << nl << ": " << inheritD + "(obj, cb != 0, excb, sentcb), response(cb)";
    }
    else
    {
        H << nl << ": " << inheritD + "(obj, cb, excb, sentcb)";
    }
    H.dec();
    H << sb;
    H << eb;

    if(p->returnsData())
    {
        //
        // completed.
        //
        H << sp << nl << "virtual void __completed(const ::Ice::AsyncResultPtr& __result) const";
        H << sb;
        H << nl << clScope << clName << "Prx __proxy = " << clScope << clName
          << "Prx::uncheckedCast(__result->getProxy());";
        writeAllocateCode(H, outParams, ret, p->getMetaData(), 
                          _useWstring | TypeContextInParam | TypeContextAMICallPrivateEnd);
        H << nl << "try";
        H << sb;
        H << nl;
        if(!usePrivateEnd(p))
        {
            if(ret)
            {
                H << retEndArg << " = ";
            }
            H << "__proxy->end_" << p->name() << spar << outEndArgs << "__result" << epar << ';';
        }
        else
        {
            H << "__proxy->___end_" << p->name() << spar << outEndArgs << retEndArg << "__result" << epar << ';';
        }
        writeEndCode(H, outParams, ret, p->getMetaData());
        H << eb;
        H << nl << "catch(::Ice::Exception& ex)";
        H << sb;
	H.zeroIndent();
	H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
	H.restoreIndent();
        H << nl << "__exception(__result, ex);";
	H.zeroIndent();
	H << nl << "#else";
	H.restoreIndent();
        H << nl << "" << baseD << "::__exception(__result, ex);";
	H.zeroIndent();
	H << nl << "#endif";
	H.restoreIndent();
        H << nl << "return;";
        H << eb;
        H << nl << "if(response)";
        H << sb;
	H.zeroIndent();
	H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
	H.restoreIndent();
        H << nl << "(callback.get()->*response)" << spar;
        if(ret)
        {
            H << "__ret";
        }
        H << outArgs;
        if(withCookie)
        {
            H << "CT::dynamicCast(__result->getCookie())";
        }
        H << epar << ';';
	H.zeroIndent();
	H << nl << "#else";
	H.restoreIndent();
        H << nl << "(" << baseD << "::callback.get()->*response)" << spar;
        if(ret)
        {
            H << "__ret";
        }
        H << outArgs;
        if(withCookie)
        {
            H << "CT::dynamicCast(__result->getCookie())";
        }
        H << epar << ';';
	H.zeroIndent();
	H << nl << "#endif";
	H.restoreIndent();
        H << eb;
        H << eb;
        
        H << sp << nl << "Response response;";
    }
    H << eb << ';';

    // Factory method
    for(int i = 0; i < 2; i++)
    {
        string callbackT = i == 0 ? "const IceUtil::Handle<T>&" : "T*";

        if(withCookie)
        {
            cookieT = "const CT&";
            comCookieT = " , const CT&";
            H << sp << nl << "template<class T, typename CT> " << delName << "Ptr"; 
        }
        else
        {
            H << sp << nl << "template<class T> " << delName << "Ptr"; 
        }

        H << nl << "new" << delName << "(" << callbackT << " instance, ";
        if(p->returnsData())
        {
            H  << "void (T::*cb)" << spar;
            if(ret)
            {
                H << retS;
            }
            H << outDecls;
            if(withCookie)
            {
                H << cookieT;
            }
            H << epar << ", ";
        }
        else
        {
            H  << "void (T::*cb)(" << cookieT << "),";
        }
        H << "void (T::*excb)(" << "const ::Ice::Exception&" << comCookieT << "),";
        H << "void (T::*sentcb)(bool" << comCookieT << ") = 0)";
        H << sb;
        if(withCookie)
        {
            H << nl << "return new " << delTmplName << "<T, CT>(instance, cb, excb, sentcb);";
        }
        else
        {
            H << nl << "return new " << delTmplName << "<T>(instance, cb, excb, sentcb);";
        }
        H << eb;

        if(withCookie)
        {
            H << sp << nl << "template<class T, typename CT> " << delName << "Ptr"; 
        }
        else
        {
            H << sp << nl << "template<class T> " << delName << "Ptr"; 
        }
        H << nl << "new" << delName << "(" << callbackT << " instance, ";
        H << "void (T::*excb)(" << "const ::Ice::Exception&" << comCookieT << "),";
        H << "void (T::*sentcb)(bool" << comCookieT << ") = 0)";
        H << sb;
        if(withCookie)
        {
            H << nl << "return new " << delTmplName << "<T, CT>(instance, 0, excb, sentcb);";
        }
        else
        {
            H << nl << "return new " << delTmplName << "<T>(instance, 0, excb, sentcb);";
        }
        H << eb;
    }
}

Slice::Gen::IceInternalVisitor::IceInternalVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport)
{
}

bool
Slice::Gen::IceInternalVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasClassDecls())
    {
        return false;
    }

    H << sp;
    H << nl << "namespace IceInternal" << nl << '{';

    return true;
}

void
Slice::Gen::IceInternalVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp;
    H << nl << '}';
}

void
Slice::Gen::IceInternalVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    string scoped = fixKwd(p->scoped());

    H << sp;

    if(!p->isLocal())
    {
        H << nl << _dllExport << "::Ice::Object* upCast(" << scoped << "*);";
        H << nl << _dllExport << "::IceProxy::Ice::Object* upCast(::IceProxy" << scoped << "*);";
    }
    else
    {
        H << nl << _dllExport << "::Ice::LocalObject* upCast(" << scoped << "*);";
    }
}

bool
Slice::Gen::IceInternalVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    string scoped = fixKwd(p->scoped());

    C << sp;
    if(!p->isLocal())
    {
        C << nl
	  << (_dllExport.empty() ? "" : "ICE_DECLSPEC_EXPORT ")
	  << "::Ice::Object* IceInternal::upCast(" << scoped << "* p) { return p; }";
        C << nl
	  << (_dllExport.empty() ? "" : "ICE_DECLSPEC_EXPORT ")
	  << "::IceProxy::Ice::Object* IceInternal::upCast(::IceProxy" << scoped
          << "* p) { return p; }";
    }
    else
    {
        C << nl
	  << (_dllExport.empty() ? "" : "ICE_DECLSPEC_EXPORT ")
	  << "::Ice::LocalObject* IceInternal::upCast(" << scoped << "* p) { return p; }";
    }

    return true;
}

Slice::Gen::HandleVisitor::HandleVisitor(Output& h, Output& c, const string& dllExport, bool stream) :
    H(h), C(c), _dllExport(dllExport), _stream(stream)
{
}

bool
Slice::Gen::HandleVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasClassDecls())
    {
        return false;
    }

    string name = fixKwd(p->name());

    H << sp;
    H << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::HandleVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp;
    H << nl << '}';
}

void
Slice::Gen::HandleVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    string name = p->name();
    string scoped = fixKwd(p->scoped());

    H << sp;
    H << nl << "typedef ::IceInternal::Handle< " << scoped << "> " << name << "Ptr;";

    if(!p->isLocal())
    {
        H << nl << "typedef ::IceInternal::ProxyHandle< ::IceProxy" << scoped << "> " << name << "Prx;";

        H << sp;
        H << nl << _dllExport << "void __read(::IceInternal::BasicStream*, " << name << "Prx&);";
        H << nl << _dllExport << "void __patch__" << name << "Ptr(void*, ::Ice::ObjectPtr&);";
        if(_stream)
        {
            H.zeroIndent();
            H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            H.restoreIndent();
            H << nl << "//Stream api is not supported in VC++ 6";
            H.zeroIndent();
            H << nl << "#else";
            H.restoreIndent();
            H << sp;
            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_write" << name << "Prx(const ::Ice::OutputStreamPtr&, const " << name
              << "Prx&);";
            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_read" << name << "Prx(const ::Ice::InputStreamPtr&, " << name
              << "Prx&);";

            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_write" << name << "(const ::Ice::OutputStreamPtr&, const "
              << name << "Ptr&);";
            H << nl << _dllExport << "ICE_DEPRECATED_API void ice_read" << name << "(const ::Ice::InputStreamPtr&, " << name << "Ptr&);";
            H.zeroIndent();
            H << nl << "#endif";
            H.restoreIndent();
        }
    }
}

bool
Slice::Gen::HandleVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(!p->isLocal())
    {
        string name = p->name();
        string scoped = fixKwd(p->scoped());
        string scope = fixKwd(p->scope());

        string factory;
        string type;
        if(!p->isAbstract())
        {
            type = scoped + "::ice_staticId()";
            factory = scoped + "::ice_factory()";
        }
        else
        {
            type = "\"\"";
            factory = "0";
        }

        C << sp;
        C << nl << "void" << nl << scope.substr(2) << "__read(::IceInternal::BasicStream* __is, "
          << scope << name << "Prx& v)";
        C << sb;
        C << nl << "::Ice::ObjectPrx proxy;";
        C << nl << "__is->read(proxy);";
        C << nl << "if(!proxy)";
        C << sb;
        C << nl << "v = 0;";
        C << eb;
        C << nl << "else";
        C << sb;
        C << nl << "v = new ::IceProxy" << scoped << ';';
        C << nl << "v->__copyFrom(proxy);";
        C << eb;
        C << eb;

        if(_stream)
        {
            C.zeroIndent();
            C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
            C.restoreIndent();
            C << nl << "//Stream api is not supported in VC++ 6";
            C.zeroIndent();
            C << nl << "#else";
            C.restoreIndent();
            C << sp;
            C << nl << "void" << nl << scope.substr(2) << "ice_write" << name
              << "Prx(const ::Ice::OutputStreamPtr& __outS, const " << scope << name << "Prx& v)";
            C << sb;
            C << nl << "__outS->writeProxy(v);";
            C << eb;

            C << sp;
            C << nl << "void" << nl << scope.substr(2) << "ice_read" << name
              << "Prx(const ::Ice::InputStreamPtr& __inS, " << scope << name << "Prx& v)";
            C << sb;
            C << nl << "::Ice::ObjectPrx proxy = __inS->readProxy();";
            C << nl << "if(!proxy)";
            C << sb;
            C << nl << "v = 0;";
            C << eb;
            C << nl << "else";
            C << sb;
            C << nl << "v = new ::IceProxy" << scoped << ';';
            C << nl << "v->__copyFrom(proxy);";
            C << eb;
            C << eb;

            C << sp;
            C << nl << "void" << nl << scope.substr(2) << "ice_write" << name
              << "(const ::Ice::OutputStreamPtr& __outS, const " << scope << name << "Ptr& v)";
            C << sb;
            C << nl << "__outS->writeObject(v);";
            C << eb;

            C << sp;
            C << nl << "void" << nl << scope.substr(2) << "ice_read" << name << "(const ::Ice::InputStreamPtr& __inS, "
              << scope << name << "Ptr& __v)";
            C << sb;
            C << nl << "::Ice::ReadObjectCallbackPtr __cb = new ::Ice::ReadObjectCallbackI(" << scope << "__patch__"
              << name << "Ptr, &__v);";
            C << nl << "__inS->readObject(__cb);";
            C << eb;
            C.zeroIndent();
            C << nl << "#endif";
            C.restoreIndent();
        }
    }

    return true;
}

Slice::Gen::ImplVisitor::ImplVisitor(Output& h, Output& c,
                                     const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

void
Slice::Gen::ImplVisitor::writeDecl(Output& out, const string& name, const TypePtr& type, const StringList& metaData)
{
    out << nl << typeToString(type, metaData, _useWstring) << ' ' << name;

    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if(builtin)
    {
        switch(builtin->kind())
        {
            case Builtin::KindBool:
            {
                out << " = false";
                break;
            }
            case Builtin::KindByte:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindLong:
            {
                out << " = 0";
                break;
            }
            case Builtin::KindFloat:
            case Builtin::KindDouble:
            {
                out << " = 0.0";
                break;
            }
            case Builtin::KindString:
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            case Builtin::KindLocalObject:
            {
                break;
            }
        }
    }

    EnumPtr en = EnumPtr::dynamicCast(type);
    if(en)
    {
        EnumeratorList enumerators = en->getEnumerators();
        out << " = " << fixKwd(en->scope()) << fixKwd(enumerators.front()->name());
    }

    out << ';';
}

void
Slice::Gen::ImplVisitor::writeReturn(Output& out, const TypePtr& type, const StringList& metaData)
{
    BuiltinPtr builtin = BuiltinPtr::dynamicCast(type);
    if(builtin)
    {
        switch(builtin->kind())
        {
            case Builtin::KindBool:
            {
                out << nl << "return false;";
                break;
            }
            case Builtin::KindByte:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindLong:
            {
                out << nl << "return 0;";
                break;
            }
            case Builtin::KindFloat:
            case Builtin::KindDouble:
            {
                out << nl << "return 0.0;";
                break;
            }
            case Builtin::KindString:
            {
                out << nl << "return ::std::string();";
                break;
            }
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            case Builtin::KindLocalObject:
            {
                out << nl << "return 0;";
                break;
            }
        }
    }
    else
    {
        ProxyPtr prx = ProxyPtr::dynamicCast(type);
        if(prx)
        {
            out << nl << "return 0;";
        }
        else
        {
            ClassDeclPtr cl = ClassDeclPtr::dynamicCast(type);
            if(cl)
            {
                out << nl << "return 0;";
            }
            else
            {
                StructPtr st = StructPtr::dynamicCast(type);
                if(st)
                {
                    out << nl << "return " << fixKwd(st->scoped()) << "();";
                }
                else
                {
                    EnumPtr en = EnumPtr::dynamicCast(type);
                    if(en)
                    {
                        EnumeratorList enumerators = en->getEnumerators();
                        out << nl << "return " << fixKwd(en->scope()) << fixKwd(enumerators.front()->name()) << ';';
                    }
                    else
                    {
                        SequencePtr seq = SequencePtr::dynamicCast(type);
                        if(seq)
                        {
                            out << nl << "return " << typeToString(seq, metaData, _useWstring) << "();";
                        }
                        else
                        {
                            DictionaryPtr dict = DictionaryPtr::dynamicCast(type);
                            assert(dict);
                            out << nl << "return " << fixKwd(dict->scoped()) << "();";
                        }
                    }
                }
            }
        }
    }
}

bool
Slice::Gen::ImplVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasClassDefs())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    set<string> includes;
    ClassList classes = p->classes();
    for(ClassList::const_iterator q = classes.begin(); q != classes.end(); ++q)
    {
        ClassList bases = (*q)->bases();
        for(ClassList::const_iterator r = bases.begin(); r != bases.end(); ++r)
        {
            if((*r)->isAbstract())
            {
                includes.insert((*r)->name());
            }
        }
    }

    for(set<string>::const_iterator it = includes.begin(); it != includes.end(); ++it)
    {
        H << nl << "#include <" << *it << "I.h>";
    }
    
    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::ImplVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp;
    H << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::ImplVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(!p->isAbstract())
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = p->name();
    string scope = fixKwd(p->scope());
    string cls = scope.substr(2) + name + "I";
    string classScopedAMD = scope + "AMD_" + name;
    ClassList bases = p->bases();

    H << sp;
    H << nl << "class " << name << "I : ";
    H.useCurrentPosAsIndent();
    H << "virtual public " << fixKwd(name);
    for(ClassList::const_iterator q = bases.begin(); q != bases.end(); ++q)
    {
        H << ',' << nl << "virtual public " << fixKwd((*q)->scope());
        if((*q)->isAbstract())
        {
            H << (*q)->name() << "I";
        }
        else
        {
            H << fixKwd((*q)->name());
        }
    }
    H.restoreIndent();

    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    OperationList ops = p->operations();
    OperationList::const_iterator r;

    for(r = ops.begin(); r != ops.end(); ++r)
    {
        OperationPtr op = (*r);
        string opName = op->name();

        TypePtr ret = op->returnType();
        string retS = returnTypeToString(ret, op->getMetaData(), _useWstring);

        if(!p->isLocal() && (p->hasMetaData("amd") || op->hasMetaData("amd")))
        {
            H << sp << nl << "virtual void " << opName << "_async(";
            H.useCurrentPosAsIndent();
            H << "const " << classScopedAMD << '_' << opName << "Ptr&";
            ParamDeclList paramList = op->parameters();
            ParamDeclList::const_iterator q;
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if(!(*q)->isOutParam())
                {
                    H << ',' << nl << inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring);
                }
            }
            H << ',' << nl << "const Ice::Current&";
            H.restoreIndent();

            bool isConst = (op->mode() == Operation::Nonmutating) || op->hasMetaData("cpp:const");

            H << ")" << (isConst ? " const" : "") << ';';

            C << sp << nl << "void" << nl << scope << name << "I::" << opName << "_async(";
            C.useCurrentPosAsIndent();
            C << "const " << classScopedAMD << '_' << opName << "Ptr& " << opName << "CB";
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if(!(*q)->isOutParam())
                {
                    C << ',' << nl << inputTypeToString((*q)->type(), (*q)->getMetaData(), _useWstring) << ' '
                      << fixKwd((*q)->name());
                }
            }
            C << ',' << nl << "const Ice::Current& current";
            C.restoreIndent();
            C << ")" << (isConst ? " const" : "");
            C << sb;

            string result = "r";
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if((*q)->name() == result)
                {
                    result = "_" + result;
                    break;
                }
            }
            if(ret)
            {
                writeDecl(C, result, ret, op->getMetaData());
            }
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if((*q)->isOutParam())
                {
                    writeDecl(C, fixKwd((*q)->name()), (*q)->type(), (*q)->getMetaData());
                }
            }

            C << nl << opName << "CB->ice_response(";
            if(ret)
            {
                C << result;
            }
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if((*q)->isOutParam())
                {
                    if(ret || q != paramList.begin())
                    {
                        C << ", ";
                    }
                    C << fixKwd((*q)->name());
                }
            }
            C << ");";

            C << eb;
        }
        else
        {
            H << sp << nl << "virtual " << retS << ' ' << fixKwd(opName) << '(';
            H.useCurrentPosAsIndent();
            ParamDeclList paramList = op->parameters();
            ParamDeclList::const_iterator q;
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if(q != paramList.begin())
                {
                    H << ',' << nl;
                }
                StringList metaData = (*q)->getMetaData();
#if defined(__SUNPRO_CC) && (__SUNPRO_CC==0x550)
                //
                // Work around for Sun CC 5.5 bug #4853566
                //
                string typeString;
                if((*q)->isOutParam())
                {
                    typeString = outputTypeToString((*q)->type(), metaData, _useWstring);
                }
                else
                {
                    typeString = inputTypeToString((*q)->type(), metaData, _useWstring);
                }
#else
                string typeString = (*q)->isOutParam() ? outputTypeToString((*q)->type(), metaData, _useWstring)
                                                       : inputTypeToString((*q)->type(), metaData, _useWstring);
#endif
                H << typeString;
            }
            if(!p->isLocal())
            {
                if(!paramList.empty())
                {
                    H << ',' << nl;
                }
                H << "const Ice::Current&";
            }
            H.restoreIndent();

            bool isConst = (op->mode() == Operation::Nonmutating) || op->hasMetaData("cpp:const");

            H << ")" << (isConst ? " const" : "") << ';';

            C << sp << nl << retS << nl;
            C << scope.substr(2) << name << "I::" << fixKwd(opName) << '(';
            C.useCurrentPosAsIndent();
            for(q = paramList.begin(); q != paramList.end(); ++q)
            {
                if(q != paramList.begin())
                {
                    C << ',' << nl;
                }
                StringList metaData = (*q)->getMetaData();
#if defined(__SUNPRO_CC) && (__SUNPRO_CC==0x550)
                //
                // Work around for Sun CC 5.5 bug #4853566
                //
                string typeString;
                if((*q)->isOutParam())
                {
                    typeString = outputTypeToString((*q)->type(), _useWstring, metaData);
                }
                else
                {
                    typeString = inputTypeToString((*q)->type(), _useWstring, metaData);
                }
#else
                string typeString = (*q)->isOutParam() ? outputTypeToString((*q)->type(), metaData, _useWstring)
                                                       : inputTypeToString((*q)->type(), metaData, _useWstring);
#endif
                C << typeString << ' ' << fixKwd((*q)->name());
            }
            if(!p->isLocal())
            {
                if(!paramList.empty())
                {
                    C << ',' << nl;
                }
                C << "const Ice::Current& current";
            }
            C.restoreIndent();
            C << ')';
            C << (isConst ? " const" : "");
            C << sb;

            if(ret)
            {
                writeReturn(C, ret, op->getMetaData());
            }

            C << eb;
        }
    }

    H << eb << ';';

    _useWstring = resetUseWstring(_useWstringHist);

    return true;
}

Slice::Gen::AsyncVisitor::AsyncVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::AsyncVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDecls() || (!p->hasContentsWithMetaData("ami") && !p->hasContentsWithMetaData("amd")))
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::AsyncVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::AsyncVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);
    return true;
}

void
Slice::Gen::AsyncVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::AsyncVisitor::visitOperation(const OperationPtr& p)
{
    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);

    if(cl->isLocal() ||
       (!cl->hasMetaData("ami") && !p->hasMetaData("ami") && !cl->hasMetaData("amd") && !p->hasMetaData("amd")))
    {
        return;
    }

    string name = p->name();

    string className = cl->name();
    string classNameAMI = "AMI_" + className;
    string classNameAMD = "AMD_" + className;
    string classScope = fixKwd(cl->scope());
    string classScopedAMI = classScope + classNameAMI;
    string classScopedAMD = classScope + classNameAMD;
    string proxyName = classScope + className + "Prx";

    vector<string> params;
    vector<string> paramsAMD;
    vector<string> paramsDecl;
    vector<string> args;

    vector<string> paramsInvoke;
    vector<string> paramsDeclInvoke;

    paramsInvoke.push_back("const " + proxyName + "&");
    paramsDeclInvoke.push_back("const " + proxyName + "& __prx");

    TypePtr ret = p->returnType();
    string retS = inputTypeToString(ret, p->getMetaData(), _useWstring);

    if(ret)
    {
        params.push_back(retS);
        paramsAMD.push_back(inputTypeToString(ret, p->getMetaData(), _useWstring));
        paramsDecl.push_back(retS + " __ret");
        args.push_back("__ret");
    }

    ParamDeclList inParams;
    ParamDeclList outParams;
    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        string paramName = fixKwd((*q)->name());
        TypePtr type = (*q)->type();
        string typeString = inputTypeToString(type, (*q)->getMetaData(), _useWstring);

        if((*q)->isOutParam())
        {
            params.push_back(typeString);
            paramsAMD.push_back(inputTypeToString(type, (*q)->getMetaData(), _useWstring));
            paramsDecl.push_back(typeString + ' ' + paramName);
            args.push_back(paramName);

            outParams.push_back(*q);
        }
        else
        {
            paramsInvoke.push_back(typeString);
            paramsDeclInvoke.push_back(typeString + ' ' + paramName);

            inParams.push_back(*q);
        }
    }

    paramsInvoke.push_back("const ::Ice::Context*");
    paramsDeclInvoke.push_back("const ::Ice::Context* __ctx");

    if(cl->hasMetaData("ami") || p->hasMetaData("ami"))
    {
        H << sp << nl << "class " << _dllExport << classNameAMI << '_' << name <<" : public ::Ice::AMICallbackBase";
        H << sb;
        H.dec();
        H << nl << "public:";
        H.inc();
        H << sp;
        H << nl << "virtual void ice_response" << spar << params << epar << " = 0;";
        H << sp;
        H << nl << "void __response" << spar << paramsDecl << epar;
        H << sb;
        H << nl << "ice_response" << spar << args << epar << ';';
        H << eb;
        H << nl << "void __exception(const ::Ice::Exception& ex)";
        H << sb;
        H << nl << "ice_exception(ex);";
        H << eb;
        H << nl << "void __sent(bool sentSynchronously)";
        H << sb;
        H << nl << "AMICallbackBase::__sent(sentSynchronously);";
        H << eb;
        H << eb << ';';
        H << sp << nl << "typedef ::IceUtil::Handle< " << classScopedAMI << '_' << name << "> " << classNameAMI
          << '_' << name  << "Ptr;";
    }

    if(cl->hasMetaData("amd") || p->hasMetaData("amd"))
    {
        H << sp << nl << "class " << _dllExport << classNameAMD << '_' << name
          << " : virtual public ::Ice::AMDCallback";
        H << sb;
        H.dec();
        H << nl << "public:";
        H.inc();
        H << sp;
        H << nl << "virtual void ice_response" << spar << paramsAMD << epar << " = 0;";
        H << eb << ';';
        H << sp << nl << "typedef ::IceUtil::Handle< " << classScopedAMD << '_' << name << "> "
          << classNameAMD << '_' << name  << "Ptr;";
    }
}

Slice::Gen::AsyncImplVisitor::AsyncImplVisitor(Output& h, Output& c, const string& dllExport) :
    H(h), C(c), _dllExport(dllExport), _useWstring(false)
{
}

bool
Slice::Gen::AsyncImplVisitor::visitUnitStart(const UnitPtr& p)
{
    if(!p->hasNonLocalClassDecls() || !p->hasContentsWithMetaData("amd"))
    {
        return false;
    }

    H << sp << nl << "namespace IceAsync" << nl << '{';

    return true;
}

void
Slice::Gen::AsyncImplVisitor::visitUnitEnd(const UnitPtr& p)
{
    H << sp << nl << '}';
}

bool
Slice::Gen::AsyncImplVisitor::visitModuleStart(const ModulePtr& p)
{
    if(!p->hasNonLocalClassDecls() || !p->hasContentsWithMetaData("amd"))
    {
        return false;
    }

    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);

    string name = fixKwd(p->name());

    H << sp << nl << "namespace " << name << nl << '{';

    return true;
}

void
Slice::Gen::AsyncImplVisitor::visitModuleEnd(const ModulePtr& p)
{
    H << sp << nl << '}';

    _useWstring = resetUseWstring(_useWstringHist);
}

bool
Slice::Gen::AsyncImplVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    _useWstring = setUseWstring(p, _useWstringHist, _useWstring);
    return true;
}

void
Slice::Gen::AsyncImplVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    _useWstring = resetUseWstring(_useWstringHist);
}

void
Slice::Gen::AsyncImplVisitor::visitOperation(const OperationPtr& p)
{
    ContainerPtr container = p->container();
    ClassDefPtr cl = ClassDefPtr::dynamicCast(container);

    if(cl->isLocal() || (!cl->hasMetaData("amd") && !p->hasMetaData("amd")))
    {
        return;
    }

    string name = p->name();

    string classNameAMD = "AMD_" + cl->name();
    string classScope = fixKwd(cl->scope());
    string classScopedAMD = classScope + classNameAMD;

    string params;
    string paramsDecl;
    string args;

    ExceptionList throws = p->throws();
    throws.sort();
    throws.unique();

    //
    // Arrange exceptions into most-derived to least-derived order. If we don't
    // do this, a base exception handler can appear before a derived exception
    // handler, causing compiler warnings and resulting in the base exception
    // being marshaled instead of the derived exception.
    //
#if defined(__SUNPRO_CC)
    throws.sort(derivedToBaseCompare);
#else
    throws.sort(Slice::DerivedToBaseCompare());
#endif

    TypePtr ret = p->returnType();
    string retS = inputTypeToString(ret, p->getMetaData(), _useWstring);

    if(ret)
    {
        params += retS;
        paramsDecl += retS;
        paramsDecl += ' ';
        paramsDecl += "__ret";
        args += "__ret";
    }

    ParamDeclList outParams;
    ParamDeclList paramList = p->parameters();
    for(ParamDeclList::const_iterator q = paramList.begin(); q != paramList.end(); ++q)
    {
        if((*q)->isOutParam())
        {
            string paramName = fixKwd((*q)->name());
            TypePtr type = (*q)->type();
            string typeString = inputTypeToString(type, (*q)->getMetaData(), _useWstring);

            if(ret || !outParams.empty())
            {
                params += ", ";
                paramsDecl += ", ";
                args += ", ";
            }

            params += typeString;
            paramsDecl += typeString;
            paramsDecl += ' ';
            paramsDecl += paramName;
            args += paramName;

            outParams.push_back(*q);
        }
    }

    H << sp << nl << "class " << _dllExport << classNameAMD << '_' << name
      << " : public " << classScopedAMD  << '_' << name << ", public ::IceInternal::IncomingAsync";
    H << sb;
    H.dec();
    H << nl << "public:";
    H.inc();

    H << sp;
    H << nl << classNameAMD << '_' << name << "(::IceInternal::Incoming&);";

    H << sp;
    H << nl << "virtual void ice_response(" << params << ");";
    if(!throws.empty())
    {
        H << nl << "virtual void ice_exception(const ::std::exception&);";

        H.zeroIndent();
        H << nl << "#if defined(__BCPLUSPLUS__)";
        H.restoreIndent();
        H << nl << "// COMPILERFIX: Avoid compiler warnings with C++Builder 2010";
        H << nl << "virtual void ice_exception()";
        H << sb;
        H << nl << "::IceInternal::IncomingAsync::ice_exception();";
        H << eb;
        H.zeroIndent();
        H << nl << "#endif";
        H.restoreIndent();
    }
    H << eb << ';';

    C << sp << nl << "IceAsync" << classScopedAMD << '_' << name << "::" << classNameAMD << '_' << name
      << "(::IceInternal::Incoming& in) :";
    C.inc();
    C << nl << "::IceInternal::IncomingAsync(in)";
    C.dec();
    C << sb;
    C << eb;

    C << sp << nl << "void" << nl << "IceAsync" << classScopedAMD << '_' << name << "::ice_response("
      << paramsDecl << ')';
    C << sb;
    C << nl << "if(__validateResponse(true))";
    C << sb;
    if(ret || !outParams.empty())
    {
        C << nl << "try";
        C << sb;
        C << nl << "::IceInternal::BasicStream* __os = this->__os();";
        writeMarshalCode(C, outParams, 0, StringList(), true);
        if(ret)
        {
            writeMarshalUnmarshalCode(C, ret, "__ret", true, "", true, p->getMetaData(), true);
        }
        if(p->returnsClasses())
        {
            C << nl << "__os->writePendingObjects();";
        }
        C << eb;
        C << nl << "catch(const ::Ice::Exception& __ex)";
        C << sb;
        C << nl << "__exception(__ex);";
        C << nl << "return;";
        C << eb;
    }
    C << nl << "__response(true);";
    C << eb;
    C << eb;

    if(!throws.empty())
    {
        C << sp << nl << "void" << nl << "IceAsync" << classScopedAMD << '_' << name
            << "::ice_exception(const ::std::exception& ex)";
        C << sb;
        ExceptionList::const_iterator r;
        for(r = throws.begin(); r != throws.end(); ++r)
        {
            C << nl;
            if(r != throws.begin())
            {
                C << "else ";
            }
            C << "if(const " << fixKwd((*r)->scoped()) << "* __ex = dynamic_cast<const " << fixKwd((*r)->scoped())
            << "*>(&ex))";
            C << sb;
            C << nl <<"if(__validateResponse(false))";
            C << sb;
            C << nl << "__os()->write(*__ex);";
            C << nl << "__response(false);";
            C << eb;
            C << eb;
        }
        C << nl << "else";
        C << sb;
        C.zeroIndent();
        C << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
        C.restoreIndent();
        C << nl << "IncomingAsync::ice_exception(ex);";
        C.zeroIndent();
        C << nl << "#else";
        C.restoreIndent();
        C << nl << "::IceInternal::IncomingAsync::ice_exception(ex);";
        C.zeroIndent();
        C << nl << "#endif";
        C.restoreIndent();
        C << eb;
        C << eb;
    }
}

Slice::Gen::StreamVisitor::StreamVisitor(Output& h, Output& c) :
    H(h), C(c)
{

}

bool
Slice::Gen::StreamVisitor::visitModuleStart(const ModulePtr&)
{
    H.zeroIndent();
    H << nl << "#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug"; // COMPILERBUG
    H << nl << "#else";
    H.restoreIndent();
    H << nl << "namespace Ice" << sb;
    return true;
}

void
Slice::Gen::StreamVisitor::visitModuleEnd(const ModulePtr&)
{
    H << eb;
    H << nl;
    H.zeroIndent();
    H << nl << "#endif";
    H.restoreIndent();
}

bool
Slice::Gen::StreamVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    if(!p->isLocal())
    {
        string scoped = p->scoped();
        H << nl << sp << "template<>";
        H << nl << "struct StreamTrait< " << fixKwd(scoped) << " >" << nl;
        H << sb;
        H << nl << "static const ::Ice::StreamTraitType type = ::Ice::StreamTraitTypeUserException;" << nl;
        H << nl << "static const int enumLimit = 0;";
        H << eb << ";" << nl;
    }
    return true;
}

void
Slice::Gen::StreamVisitor::visitExceptionEnd(const ExceptionPtr&)
{
}

bool
Slice::Gen::StreamVisitor::visitStructStart(const StructPtr& p)
{
    if(!p->isLocal())
    {
        bool classMetaData = findMetaData(p->getMetaData(), false) == "class";
        string scoped = p->scoped();
        H << nl << sp << "template<>";
        if(classMetaData)
        {
            H << nl << "struct StreamTrait< " << fixKwd(scoped + "Ptr") << " >" << nl;
        }
        else
        {
            H << nl << "struct StreamTrait< " << fixKwd(scoped) << " >" << nl;
        }
        H << sb;
        if(classMetaData)
        {
          H << nl << "static const ::Ice::StreamTraitType type = ::Ice::StreamTraitTypeStructClass;" << nl;
        }
        else
        {
            H << nl << "static const ::Ice::StreamTraitType type = ::Ice::StreamTraitTypeStruct;";
        }
        H << nl << "static const int enumLimit = 0;";
        H << eb << ";" << nl;
    }
    return true;
}

void
Slice::Gen::StreamVisitor::visitStructEnd(const StructPtr&)
{
}

void
Slice::Gen::StreamVisitor::visitEnum(const EnumPtr& p)
{
    string scoped = fixKwd(p->scoped());
    H << nl << sp << "template<>" << nl
        << "struct StreamTrait< " << scoped << ">" << nl
        << sb << nl
        << "static const ::Ice::StreamTraitType type = ";

    size_t sz = p->getEnumerators().size();
    if(sz <= 127)
    {
        H << "::Ice::StreamTraitTypeByteEnum;";
    }
    else if(sz <= 32767)
    {
        H << "::Ice::StreamTraitTypeShortEnum;";
    }
    else
    {
        H << "::Ice::StreamTraitTypeIntEnum;";
    }
    H << nl << "static const int enumLimit = " << sz << ";"
        << eb << ";" << nl;
}

void
Slice::Gen::validateMetaData(const UnitPtr& u)
{
    MetaDataVisitor visitor;
    u->visit(&visitor, false);
}

bool
Slice::Gen::MetaDataVisitor::visitUnitStart(const UnitPtr& p)
{
    static const string prefix = "cpp:";

    //
    // Validate global metadata in the top-level file and all included files.
    //
    StringList files = p->allFiles();

    for(StringList::iterator q = files.begin(); q != files.end(); ++q)
    {
        string file = *q;
        DefinitionContextPtr dc = p->findDefinitionContext(file);
        assert(dc);
        StringList globalMetaData = dc->getMetaData();
        int headerExtension = 0;
        for(StringList::const_iterator r = globalMetaData.begin(); r != globalMetaData.end(); ++r)
        {
            string s = *r;
            if(_history.count(s) == 0)
            {
                if(s.find(prefix) == 0)
                {
                    static const string cppIncludePrefix = "cpp:include:";
                    static const string cppHeaderExtPrefix = "cpp:header-ext:";
                    if(s.find(cppIncludePrefix) == 0 && s.size() > cppIncludePrefix.size())
                    {
                        continue;
                    }
                    else if(s.find(cppHeaderExtPrefix) == 0 && s.size() > cppHeaderExtPrefix.size())
                    {
                        headerExtension++;
                        if(headerExtension > 1)
                        {
                            ostringstream ostr;
                            ostr << "ignoring invalid global metadata `" << s
                                << "': directive can appear only once per file";
                            emitWarning(file, -1, ostr.str());
                            _history.insert(s);
                        }
                        continue;
                    }
                    ostringstream ostr;
                    ostr << "ignoring invalid global metadata `" << s << "'";
                    emitWarning(file, -1, ostr.str());
                }
                _history.insert(s);
            }
        }
    }

    return true;
}

bool
Slice::Gen::MetaDataVisitor::visitModuleStart(const ModulePtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
    return true;
}

void
Slice::Gen::MetaDataVisitor::visitModuleEnd(const ModulePtr&)
{
}

void
Slice::Gen::MetaDataVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
}

bool
Slice::Gen::MetaDataVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
    return true;
}

void
Slice::Gen::MetaDataVisitor::visitClassDefEnd(const ClassDefPtr&)
{
}

bool
Slice::Gen::MetaDataVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
    return true;
}

void
Slice::Gen::MetaDataVisitor::visitExceptionEnd(const ExceptionPtr&)
{
}

bool
Slice::Gen::MetaDataVisitor::visitStructStart(const StructPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
    return true;
}

void
Slice::Gen::MetaDataVisitor::visitStructEnd(const StructPtr&)
{
}

void
Slice::Gen::MetaDataVisitor::visitOperation(const OperationPtr& p)
{

    bool ami = false;
    ClassDefPtr cl = ClassDefPtr::dynamicCast(p->container());
    if(cl->hasMetaData("ami") || p->hasMetaData("ami") || cl->hasMetaData("amd") || p->hasMetaData("amd"))
    {
        ami = true;
    }

    if(p->hasMetaData("UserException"))
    {
        if(!cl->isLocal())
        {
            ostringstream ostr;
            ostr << "ignoring invalid metadata `UserException': directive applies only to local operations "
                 << "but enclosing " << (cl->isInterface() ? "interface" : "class") << "`" << cl->name()
                 << "' is not local";
            emitWarning(p->file(), p->line(), ostr.str());
        }
    }

    StringList metaData = p->getMetaData();
    metaData.remove("cpp:const");

    TypePtr returnType = p->returnType();
    if(!metaData.empty())
    {
        if(!returnType)
        {
            for(StringList::const_iterator q = metaData.begin(); q != metaData.end(); ++q)
            {
                if(q->find("cpp:type:", 0) == 0 || q->find("cpp:array", 0) == 0 || q->find("cpp:range", 0) == 0)
                {
                    emitWarning(p->file(), p->line(), "ignoring invalid metadata `" + *q +
                                "' for operation with void return type");
                    break;
                }
            }
        }
        else
        {
            validate(returnType, metaData, p->file(), p->line(), ami);
        }
    }

    ParamDeclList params = p->parameters();
    for(ParamDeclList::iterator q = params.begin(); q != params.end(); ++q)
    {
        validate((*q)->type(), (*q)->getMetaData(), p->file(), (*q)->line(), ami || !(*q)->isOutParam());
    }
}

void
Slice::Gen::MetaDataVisitor::visitParamDecl(const ParamDeclPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
}

void
Slice::Gen::MetaDataVisitor::visitDataMember(const DataMemberPtr& p)
{
    validate(p->type(), p->getMetaData(), p->file(), p->line());
}

void
Slice::Gen::MetaDataVisitor::visitSequence(const SequencePtr& p)
{
    StringList metaData = p->getMetaData();
    const string file = p->file();
    const string line = p->line();
    static const string prefix = "cpp:protobuf";
    for(StringList::const_iterator q = metaData.begin(); q != metaData.end(); )
    {
        string s = *q++;
        if(_history.count(s) == 0)
        {
            if(s.find(prefix) == 0)
            {
                //
                // Remove from list so validate does not try to handle as well.
                //
                metaData.remove(s);

                BuiltinPtr builtin = BuiltinPtr::dynamicCast(p->type());
                if(!builtin || builtin->kind() != Builtin::KindByte)
                {
                    _history.insert(s);
                    emitWarning(file, line, "ignoring invalid metadata `" + s + "':\n"+
                                "`protobuf' encoding must be a byte sequence.");
                }
            }
        }
    }

    validate(p, metaData, file, line);
}

void
Slice::Gen::MetaDataVisitor::visitDictionary(const DictionaryPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
}

void
Slice::Gen::MetaDataVisitor::visitEnum(const EnumPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
}

void
Slice::Gen::MetaDataVisitor::visitConst(const ConstPtr& p)
{
    validate(p, p->getMetaData(), p->file(), p->line());
}

void
Slice::Gen::MetaDataVisitor::validate(const SyntaxTreeBasePtr& cont, const StringList& metaData,
                                      const string& file, const string& line, bool inParam)
{
    static const string prefix = "cpp:";
    for(StringList::const_iterator p = metaData.begin(); p != metaData.end(); ++p)
    {
        string s = *p;
        if(_history.count(s) == 0)
        {
            if(s.find(prefix) == 0)
            {
                string ss = s.substr(prefix.size());
                if(ss.find("type:wstring") == 0 || ss.find("type:string") == 0)
                {
                    BuiltinPtr builtin = BuiltinPtr::dynamicCast(cont);
                    ModulePtr module = ModulePtr::dynamicCast(cont);
                    ClassDefPtr clss = ClassDefPtr::dynamicCast(cont);
                    StructPtr strct = StructPtr::dynamicCast(cont);
                    ExceptionPtr exception = ExceptionPtr::dynamicCast(cont);
                    if((builtin && builtin->kind() == Builtin::KindString) || module || clss || strct || exception)
                    {
                        continue;
                    }
                }
                if(SequencePtr::dynamicCast(cont))
                {
                    if(ss.find("type:") == 0 || (inParam && (ss == "array" || ss.find("range") == 0)))
                    {
                        continue;
                    }
                }
                if(StructPtr::dynamicCast(cont) && ss.find("class") == 0)
                {
                    continue;
                }
                if(ClassDefPtr::dynamicCast(cont) && ss.find("virtual") == 0)
                {
                    continue;
                }
                emitWarning(file, line, "ignoring invalid metadata `" + s + "'");
            }
            _history.insert(s);
        }
    }
}

int
Slice::Gen::setUseWstring(ContainedPtr p, list<int>& hist, int use)
{
    hist.push_back(use);
    StringList metaData = p->getMetaData();
    if(find(metaData.begin(), metaData.end(), "cpp:type:wstring") != metaData.end())
    {
        use = TypeContextUseWstring;
    }
    else if(find(metaData.begin(), metaData.end(), "cpp:type:string") != metaData.end())
    {
        use = 0;
    }
    return use;
}

int
Slice::Gen::resetUseWstring(list<int>& hist)
{
    int use = hist.back();
    hist.pop_back();
    return use;
}

string
Slice::Gen::getHeaderExt(const string& file, const UnitPtr& unit)
{
    string ext;
    static const string headerExtPrefix = "cpp:header-ext:";
    DefinitionContextPtr dc = unit->findDefinitionContext(file);
    assert(dc);
    string meta = dc->findMetaData(headerExtPrefix);
    if(meta.size() > headerExtPrefix.size())
    {
        ext = meta.substr(headerExtPrefix.size());
    }
    return ext;
}
