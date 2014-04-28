/*
 *    This file is part of KDevelop
 *
 *    Copyright 2013 Olivier de Gaalon <olivier.jg@gmail.com>
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to
 *    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *    Boston, MA 02110-1301, USA.
 */

#ifndef TUDUCHAIN_H
#define TUDUCHAIN_H

#include "duchainexport.h"
#include "templatehelpers.h"
#include "cursorkindtraits.h"
#include "clanghelpers.h"
#include "clangtypes.h"

#include <util/pushvalue.h>

#include <language/duchain/duchainlock.h>
#include <language/duchain/classdeclaration.h>

#include <language/duchain/types/pointertype.h>
#include <language/duchain/types/arraytype.h>
#include <language/duchain/types/referencetype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/enumerationtype.h>
#include <language/duchain/types/typealiastype.h>

#include <unordered_map>

template<CXCursorKind CK, bool isDefinition, bool isClassMember, class Enable = void>
struct DeclType;
template<CXCursorKind CK, class Enable = void>
struct IdType;

class KDEVCLANGDUCHAIN_EXPORT TUDUChain
{
public:
    TUDUChain(CXTranslationUnit tu, CXFile file, const IncludeFileContexts& includes, const bool update);

private:
    static CXChildVisitResult visitCursor(CXCursor cursor, CXCursor parent, CXClientData data);

    void setIdTypeDecl(CXCursor typeCursor, IdentifiedType *idType) const;
    void contextImportDecl(DUContext *context, const DeclarationPointer &decl) const;

    KDevelop::RangeInRevision makeContextRange(CXCursor cursor) const;
    KDevelop::Identifier makeId(CXCursor cursor) const;
    QByteArray makeComment(CXComment comment) const;
    AbstractType *makeType(CXType type) const;
    AbstractType::Ptr makeAbsType(CXType type) const { return AbstractType::Ptr(makeType(type)); }

//BEGIN dispatch*
    template<CXCursorKind CK, EnableIf<CursorKindTraits::isUse(CK)> = dummy>
    CXChildVisitResult dispatchCursor(CXCursor cursor)
    {
        return buildUse<CK>(cursor);
    }

    template<
      CXCursorKind CK,
      Decision IsInClass = CursorKindTraits::isInClass(CK),
      EnableIf<IsInClass == Decision::Maybe> = dummy>
    CXChildVisitResult dispatchCursor(CXCursor cursor, CXCursor parent)
    {
      const bool decision = CursorKindTraits::isClass(clang_getCursorKind(parent));
      return decision ?
        dispatchCursor<CK, Decision::True, CursorKindTraits::isDefinition(CK, Decision::True)>(cursor, parent) :
        dispatchCursor<CK, Decision::False, CursorKindTraits::isDefinition(CK, Decision::False)>(cursor, parent);
    }

    template<
        CXCursorKind CK,
        Decision IsInClass = CursorKindTraits::isInClass(CK),
        Decision IsDefinition = CursorKindTraits::isDefinition(CK, IsInClass),
        EnableIf<IsDefinition == Decision::Maybe && IsInClass != Decision::Maybe> = dummy>
    CXChildVisitResult dispatchCursor(CXCursor cursor, CXCursor parent)
    {
        const bool decision = clang_isCursorDefinition(cursor);
        return decision ?
          dispatchCursor<CK, IsInClass, Decision::True>(cursor, parent) :
          dispatchCursor<CK, IsInClass, Decision::False>(cursor, parent);
    }

    template<
        CXCursorKind CK,
        Decision IsInClass = CursorKindTraits::isInClass(CK),
        Decision IsDefinition = CursorKindTraits::isDefinition(CK, IsInClass),
        EnableIf<IsInClass != Decision::Maybe && IsDefinition != Decision::Maybe> = dummy>
    CXChildVisitResult dispatchCursor(CXCursor cursor, CXCursor /*parent*/)
    {
        constexpr bool isClassMember = IsInClass == Decision::True;
        constexpr bool isDefinition = IsDefinition == Decision::True;
        //Currently, but not technically, hasContext and IsDefinition are synonyms
        constexpr bool hasContext = IsDefinition == Decision::True;
        return buildDeclaration<CK, typename DeclType<CK, isDefinition, isClassMember>::Type, hasContext>(cursor);
    }

    template<CXTypeKind TK>
    AbstractType *dispatchType(CXType type)  const
    {
        auto kdevType = createType<TK>(type);
        setTypeModifiers<TK>(type, kdevType);
        return kdevType;
    }
//BEGIN dispatch*

//BEGIN build*
    template<CXCursorKind CK, class DeclType, bool hasContext>
    CXChildVisitResult buildDeclaration(CXCursor cursor)
    {
        auto id = makeId(cursor);
        if (hasContext) {
            auto context = createContext<CK, CursorKindTraits::contextType(CK)>(cursor, id);
            createDeclaration<CK, DeclType>(cursor, id, context);
            CurrentContext newParent(context);
            PushValue<CurrentContext*> pushCurrent(m_parentContext, &newParent);
            clang_visitChildren(cursor, &visitCursor, this);
            return CXChildVisit_Continue;
        }
        createDeclaration<CK, DeclType>(cursor, id, nullptr);
        return CXChildVisit_Recurse;
    }

    template<CXCursorKind CK>
    CXChildVisitResult buildUse(CXCursor cursor)
    {
        m_uses[m_parentContext->context].push_back(cursor);
        return CK == CXCursor_DeclRefExpr || CK == CXCursor_MemberRefExpr ?
            CXChildVisit_Recurse : CXChildVisit_Continue;
    }
//END build*

//BEGIN create*
    template<CXCursorKind CK, class DeclType>
    DeclType* createDeclarationCommon(CXCursor cursor, const Identifier& id)
    {
        auto range = ClangRange(clang_Cursor_getSpellingNameRange(cursor, 0, 0)).toRangeInRevision();
        if (m_update) {
            const IndexedIdentifier indexedId(id);
            DUChainWriteLocker lock;
            auto it = m_parentContext->previousChildDeclarations.begin();
            while (it != m_parentContext->previousChildDeclarations.end()) {
                auto decl = dynamic_cast<DeclType*>(*it);
                if (decl && decl->indexedIdentifier() == indexedId) {
                    decl->setRange(range);
                    setDeclData<CK>(cursor, decl);
                    m_parentContext->previousChildDeclarations.erase(it);
                    return decl;
                }
                ++it;
            }
        }
        auto decl = new DeclType(range, nullptr);
        decl->setIdentifier(id);
        setDeclData<CK>(cursor, decl);
        return decl;
    }

    template<CXCursorKind CK, class DeclType>
    KDevelop::Declaration* createDeclaration(CXCursor cursor, const KDevelop::Identifier& id, KDevelop::DUContext *context)
    {
        auto decl = createDeclarationCommon<CK, DeclType>(cursor, id);
        auto type = createType<CK>(cursor);
        DUChainWriteLocker lock;
        decl->setContext(m_parentContext->context);
        if (context)
            decl->setInternalContext(context);
        setDeclType<CK>(decl, type);
        setDeclInCtxtData<CK>(cursor, decl);
        return decl;
    }

    template<CXCursorKind CK, KDevelop::DUContext::ContextType Type>
    KDevelop::DUContext* createContext(CXCursor cursor, const KDevelop::Identifier& id)
    {
        // wtf: why is the DUContext API requesting a QID when it needs a plain Id?!
        // see: testNamespace
        const QualifiedIdentifier scopeId(id);
        auto range = makeContextRange(cursor);
        DUChainWriteLocker lock;
        if (m_update) {
            const KDevelop::IndexedQualifiedIdentifier indexedScopeId(scopeId);
            auto it = m_parentContext->previousChildContexts.begin();
            while (it != m_parentContext->previousChildContexts.end()) {
                auto ctx = *it;
                if (ctx->type() == Type && ctx->indexedLocalScopeIdentifier() == indexedScopeId) {
                    ctx->setRange(range);
                    m_parentContext->previousChildContexts.erase(it);
                    return ctx;
                }
                ++it;
            }
        }
        //TODO: (..type, id..) constructor for DUContext?
        auto context = new KDevelop::DUContext(range, m_parentContext->context);
        context->setType(Type);
        context->setLocalScopeIdentifier(scopeId);
        if (Type == KDevelop::DUContext::Other)
            context->setInSymbolTable(false);
        if (CK == CXCursor_CXXMethod) {
            CXCursor semParent = clang_getCursorSemanticParent(cursor);
            if (!clang_Cursor_isNull(semParent)) {
                DeclarationPointer semParentDecl = findDeclaration(semParent, m_includes);
                if (semParentDecl) {
                    contextImportDecl(context, semParentDecl);
                }
            }
        }
        return context;
    }

    template<CXTypeKind TK, EnableIf<CursorKindTraits::integralType(TK) != -1> = dummy>
    AbstractType *createType(CXType) const
    {
        return new IntegralType(CursorKindTraits::integralType(TK));
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_Pointer> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto ptr = new PointerType;
        ptr->setBaseType(makeAbsType(clang_getPointeeType(type)));
        return ptr;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_ConstantArray || TK == CXType_IncompleteArray || TK == CXType_VariableArray> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto arr = new ArrayType;
        arr->setDimension((TK == CXType_IncompleteArray || TK == CXType_VariableArray) ? 0 : clang_getArraySize(type));
        arr->setElementType(makeAbsType(clang_getArrayElementType(type)));
        return arr;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_RValueReference || TK == CXType_LValueReference> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto ref = new ReferenceType;
        ref->setIsRValue(type.kind == CXType_RValueReference);
        ref->setBaseType(makeAbsType(clang_getPointeeType(type)));
        return ref;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_FunctionProto> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto func = new FunctionType;
        func->setReturnType(makeAbsType(clang_getResultType(type)));
        const int numArgs = clang_getNumArgTypes(type);
        for (int i = 0; i < numArgs; ++i) {
            func->addArgument(makeAbsType(clang_getArgType(type, i)));
        }
        /// TODO: variadic functions
        return func;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_Record> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto t = new StructureType;
        setIdTypeDecl(clang_getTypeDeclaration(type), t);
        return t;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_Enum> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto t = new EnumerationType;
        setIdTypeDecl(clang_getTypeDeclaration(type), t);
        return t;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_Typedef> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto t = new TypeAliasType;
        CXCursor location = clang_getTypeDeclaration(type);
        t->setType(makeAbsType(clang_getTypedefDeclUnderlyingType(location)));
        setIdTypeDecl(location, t);
        return t;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_Int128> = dummy>
    AbstractType *createType(CXType) const
    {
        auto t = new DelayedType;
        static const IndexedTypeIdentifier id("__int128");
        t->setIdentifier(id);
        return t;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_UInt128> = dummy>
    AbstractType *createType(CXType) const
    {
        auto t = new DelayedType;
        static const IndexedTypeIdentifier id("unsigned __int128");
        t->setIdentifier(id);
        return t;
    }

    template<CXTypeKind TK, EnableIf<TK == CXType_Vector || TK == CXType_Unexposed> = dummy>
    AbstractType *createType(CXType type) const
    {
        auto t = new DelayedType;
        t->setIdentifier(IndexedTypeIdentifier(QString::fromUtf8(ClangString(clang_getTypeSpelling(type)))));
        return t;
    }

    template<CXCursorKind CK, EnableIf<CursorKindTraits::isIdentifiedType(CK) && CK != CXCursor_TypedefDecl> = dummy>
    typename IdType<CK>::Type *createType(CXCursor) const
    {
        return new typename IdType<CK>::Type;
    }

    template<CXCursorKind CK, EnableIf<CK == CXCursor_TypedefDecl> = dummy>
    TypeAliasType *createType(CXCursor cursor) const
    {
        auto type = new TypeAliasType;
        type->setType(makeAbsType(clang_getTypedefDeclUnderlyingType(cursor)));
        return type;
    }

    template<CXCursorKind CK, EnableIf<!CursorKindTraits::isIdentifiedType(CK)> = dummy>
    AbstractType *createType(CXCursor cursor) const
    {
        auto clangType = clang_getCursorType(cursor);
        return makeType(clangType);
    }
//END create*

//BEGIN setDeclData
template<CXCursorKind CK>
void setDeclData(CXCursor cursor, Declaration *decl, bool setComment = true) const
{
    if (setComment)
        decl->setComment(makeComment(clang_Cursor_getParsedComment(cursor)));
    if (CK == CXCursor_TypeAliasDecl || CK == CXCursor_TypedefDecl)
        decl->setIsTypeAlias(true);
    if (CK == CXCursor_Namespace)
        decl->setKind(Declaration::Namespace);
    if (CK == CXCursor_EnumDecl || CK == CXCursor_EnumConstantDecl || CursorKindTraits::isClass(CK))
        decl->setKind(Declaration::Type);
}

template<CXCursorKind CK>
void setDeclData(CXCursor cursor, ClassMemberDeclaration *decl) const
{
    setDeclData<CK>(cursor, static_cast<Declaration*>(decl));
    //A CXCursor_VarDecl in a class is static (otherwise it'd be a CXCursor_FieldDecl)
    if (CK == CXCursor_VarDecl)
        decl->setStatic(true);
    decl->setAccessPolicy(CursorKindTraits::kdevAccessPolicy(clang_getCXXAccessSpecifier(cursor)));
}

template<CXCursorKind CK, EnableIf<CursorKindTraits::isClassTemplate(CK)> = dummy>
void setDeclData(CXCursor cursor, ClassDeclaration* decl) const
{
    CXCursorKind kind = clang_getTemplateCursorKind(cursor);
    switch (kind) {
        case CXCursor_UnionDecl: setDeclData<CXCursor_UnionDecl>(cursor, decl); break;
        case CXCursor_StructDecl: setDeclData<CXCursor_StructDecl>(cursor, decl); break;
        case CXCursor_ClassDecl: setDeclData<CXCursor_ClassDecl>(cursor, decl); break;
        default: Q_ASSERT(false); break;
    }
}

template<CXCursorKind CK, EnableIf<!CursorKindTraits::isClassTemplate(CK)> = dummy>
void setDeclData(CXCursor cursor, ClassDeclaration* decl) const
{
    if (m_update) {
        decl->clearBaseClasses();
    }
    setDeclData<CK>(cursor, static_cast<ClassMemberDeclaration*>(decl));
    if (CK == CXCursor_UnionDecl)
        decl->setClassType(ClassDeclarationData::Union);
    if (CK == CXCursor_StructDecl)
        decl->setClassType(ClassDeclarationData::Struct);
}

template<CXCursorKind CK>
void setDeclData(CXCursor cursor, ClassFunctionDeclaration* decl) const
{
    setDeclData<CK>(cursor, static_cast<ClassMemberDeclaration*>(decl));
    decl->setStatic(clang_CXXMethod_isStatic(cursor));
    decl->setVirtual(clang_CXXMethod_isVirtual(cursor));
}

template<CXCursorKind CK>
void setDeclData(CXCursor cursor, FunctionDefinition *decl) const
{
    bool setComment = clang_equalCursors(clang_getCanonicalCursor(cursor), cursor);
    setDeclData<CK>(cursor, static_cast<FunctionDeclaration*>(decl), setComment);
}

//END setDeclData

//BEGIN setDeclInCtxtData
template<CXCursorKind CK>
void setDeclInCtxtData(CXCursor, Declaration*)
{
    //No-op
}

template<CXCursorKind CK>
void setDeclInCtxtData(CXCursor cursor, FunctionDefinition *def)
{
    setDeclInCtxtData<CK>(cursor, static_cast<FunctionDeclaration*>(def));

    CXCursor canon = clang_getCanonicalCursor(cursor);
    if (clang_equalCursors(canon, cursor)) {
        def->setDeclarationIsDefinition(true);
    }
    else if (auto decl = findDeclaration(canon, m_includes)) {
        def->setDeclaration(decl.data());
    }
}
//END setDeclInCtxtData

//BEGIN setDeclType
    template<CXCursorKind CK>
    void setDeclType(Declaration *decl, typename IdType<CK>::Type *type)
    {
        setDeclType<CK>(decl, static_cast<IdentifiedType*>(type));
        setDeclType<CK>(decl, static_cast<AbstractType*>(type));
    }

    template<CXCursorKind CK>
    void setDeclType(Declaration *decl, IdentifiedType *type)
    {
        type->setDeclaration(decl);
    }

    template<CXCursorKind CK>
    void setDeclType(Declaration *decl, AbstractType *type)
    {
        decl->setAbstractType(AbstractType::Ptr(type));
    }
//END setDeclType

template<CXTypeKind TK>
void setTypeModifiers(CXType type, AbstractType* kdevType) const;

private:
    struct CurrentContext
    {
        CurrentContext(KDevelop::DUContext* context)
            : context(context)
        {
            DUChainReadLocker lock;
            previousChildContexts = context->childContexts();
            previousChildDeclarations = context->localDeclarations();
        }
        ~CurrentContext()
        {
            DUChainWriteLocker lock;
            qDeleteAll(previousChildContexts);
            qDeleteAll(previousChildDeclarations);
        }

        KDevelop::DUContext* context;
        // when updatig, this contains child contexts of the current parent context
        QVector<KDevelop::DUContext*> previousChildContexts;
        // when updatig, this contains child declarations of the current parent context
        QVector<KDevelop::Declaration*> previousChildDeclarations;
    };
    friend CurrentContext;

    const CXFile m_file;
    const IncludeFileContexts &m_includes;

    std::unordered_map<DUContext*, std::vector<CXCursor>> m_uses;
    CurrentContext *m_parentContext;

    const bool m_update;
};

template<>
CXChildVisitResult TUDUChain::buildUse<CXCursor_CXXBaseSpecifier>(CXCursor cursor);

#endif //TUDUCHAIN_H
