#ifndef __CLAY_HPP
#define __CLAY_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <climits>
#include <cerrno>

#include <llvm/ADT/Triple.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Host.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Intrinsics.h>

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using std::map;
using std::set;
using std::ostream;
using std::ostringstream;

#ifdef _MSC_VER
#define strtoll _strtoi64
#define strtoull _strtoui64
#define CLAY_ALIGN(n) __declspec(align(n))
#else
#define CLAY_ALIGN(n) __attribute__((aligned(n)))
#endif

#define CLAY_LANGUAGE_VERSION "0.1-WIP"


//
// Target-specific types
//
typedef int ptrdiff32_t;
typedef long long ptrdiff64_t;

typedef unsigned size32_t;
typedef unsigned long long size64_t;


//
// int128 type
//

#if (defined(_MSC_VER) && defined(_M_X64)) \
    || (defined(__GNUC__) && defined(_INT128_DEFINED))
typedef __int128 clay_int128;
typedef unsigned __int128 clay_uint128;
#elif (defined(__clang__))
typedef __int128_t clay_int128;
typedef __uint128_t clay_uint128;
#else
// fake it by doing 64-bit math in a 128-bit padded container
struct uint128_holder;

struct int128_holder {
    ptrdiff64_t lowValue;
    ptrdiff64_t highPad; // not used in static math

    int128_holder() {}
    explicit int128_holder(ptrdiff64_t low) : lowValue(low), highPad(low < 0 ? -1 : 0) {}
    int128_holder(ptrdiff64_t low, ptrdiff64_t high)
        : lowValue(low), highPad(high) {}
    explicit int128_holder(uint128_holder y);

    int128_holder &operator=(ptrdiff64_t low) {
        new ((void*)this) int128_holder(low);
        return *this;
    }

    int128_holder operator-() const { return int128_holder(-lowValue); }
    int128_holder operator~() const { return int128_holder(~lowValue); }

    bool operator==(int128_holder y) const { return lowValue == y.lowValue; }
    bool operator< (int128_holder y) const { return lowValue <  y.lowValue; }

    int128_holder operator+ (int128_holder y) const { return int128_holder(lowValue +  y.lowValue); }
    int128_holder operator- (int128_holder y) const { return int128_holder(lowValue -  y.lowValue); }
    int128_holder operator* (int128_holder y) const { return int128_holder(lowValue *  y.lowValue); }
    int128_holder operator/ (int128_holder y) const { return int128_holder(lowValue /  y.lowValue); }
    int128_holder operator% (int128_holder y) const { return int128_holder(lowValue %  y.lowValue); }
    int128_holder operator<<(int128_holder y) const { return int128_holder(lowValue << y.lowValue); }
    int128_holder operator>>(int128_holder y) const { return int128_holder(lowValue >> y.lowValue); }
    int128_holder operator& (int128_holder y) const { return int128_holder(lowValue &  y.lowValue); }
    int128_holder operator| (int128_holder y) const { return int128_holder(lowValue |  y.lowValue); }
    int128_holder operator^ (int128_holder y) const { return int128_holder(lowValue ^  y.lowValue); }

    operator ptrdiff64_t() const { return lowValue; }
} CLAY_ALIGN(16);

struct uint128_holder {
    size64_t lowValue;
    size64_t highPad; // not used in static math

    uint128_holder() {}
    explicit uint128_holder(size64_t low) : lowValue(low), highPad(0) {}
    uint128_holder(size64_t low, size64_t high) : lowValue(low), highPad(high) {}
    explicit uint128_holder(int128_holder y) : lowValue(y.lowValue), highPad(y.highPad) {}

    uint128_holder &operator=(size64_t low) {
        new ((void*)this) uint128_holder(low);
        return *this;
    }

    uint128_holder operator-() const { return uint128_holder(-lowValue); }
    uint128_holder operator~() const { return uint128_holder(~lowValue); }

    bool operator==(uint128_holder y) const { return lowValue == y.lowValue; }
    bool operator< (uint128_holder y) const { return lowValue <  y.lowValue; }

    uint128_holder operator+ (uint128_holder y) const { return uint128_holder(lowValue +  y.lowValue); }
    uint128_holder operator- (uint128_holder y) const { return uint128_holder(lowValue -  y.lowValue); }
    uint128_holder operator* (uint128_holder y) const { return uint128_holder(lowValue *  y.lowValue); }
    uint128_holder operator/ (uint128_holder y) const { return uint128_holder(lowValue /  y.lowValue); }
    uint128_holder operator% (uint128_holder y) const { return uint128_holder(lowValue %  y.lowValue); }
    uint128_holder operator<<(uint128_holder y) const { return uint128_holder(lowValue << y.lowValue); }
    uint128_holder operator>>(uint128_holder y) const { return uint128_holder(lowValue >> y.lowValue); }
    uint128_holder operator& (uint128_holder y) const { return uint128_holder(lowValue &  y.lowValue); }
    uint128_holder operator| (uint128_holder y) const { return uint128_holder(lowValue |  y.lowValue); }
    uint128_holder operator^ (uint128_holder y) const { return uint128_holder(lowValue ^  y.lowValue); }

    operator size64_t() const { return lowValue; }
} CLAY_ALIGN(16);

namespace std {
template<>
class numeric_limits<int128_holder> {
public:
    static int128_holder min() throw() {
        return int128_holder(0, std::numeric_limits<ptrdiff64_t>::min());
    }
    static int128_holder max() throw() {
        return int128_holder(-1, std::numeric_limits<ptrdiff64_t>::max());
    }
};

template<>
class numeric_limits<uint128_holder> {
public:
    static uint128_holder min() throw() {
        return uint128_holder(0, 0);
    }
    static uint128_holder max() throw() {
        return uint128_holder(
            std::numeric_limits<size64_t>::max(),
            std::numeric_limits<size64_t>::max()
        );
    }
};
}

inline int128_holder::int128_holder(uint128_holder y)
    : lowValue(y.lowValue), highPad(y.highPad) {}

typedef int128_holder clay_int128;
typedef uint128_holder clay_uint128;
#endif

inline std::ostream &operator<<(std::ostream &os, clay_int128 x) {
    return os << ptrdiff64_t(x);
}
inline std::ostream &operator<<(std::ostream &os, clay_uint128 x) {
    return os << size64_t(x);
}


//
// Pointer
//

template<class T>
class Pointer {
    T *p;
public :
    Pointer()
        : p(0) {}
    Pointer(T *p)
        : p(p) {
        if (p)
            p->incRef();
    }
    Pointer(const Pointer<T> &other)
        : p(other.p) {
        if (p)
            p->incRef();
    }
    ~Pointer() {
        if (p)
            p->decRef();
    }
    Pointer<T> &operator=(const Pointer<T> &other) {
        T *q = other.p;
        if (q) q->incRef();
        if (p) p->decRef();
        p = q;
        return *this;
    }
    T &operator*() { return *p; }
    const T &operator*() const { return *p; }
    T *operator->() { return p; }
    const T *operator->() const { return p; }
    T *ptr() const { return p; }
    bool operator!() const { return p == 0; }
    bool operator==(const Pointer<T> &other) const {
        return p == other.p;
    }
    bool operator!=(const Pointer<T> &other) const {
        return p != other.p;
    }
    bool operator<(const Pointer<T> &other) const {
        return p < other.p;
    }
};



//
// Object
//

struct Object {
    int refCount;
    int objKind;
    Object(int objKind)
        : refCount(0), objKind(objKind) {}
    virtual ~Object() {}
    void incRef() { ++refCount; }
    void decRef() {
        if (--refCount == 0)
            delete this;
    }
};

typedef Pointer<Object> ObjectPtr;



//
// ObjectKind
//

enum ObjectKind {
    SOURCE,
    LOCATION,
    TOKEN,

    IDENTIFIER,
    DOTTED_NAME,

    EXPRESSION,
    EXPR_LIST,
    STATEMENT,
    CASE_BLOCK,
    CATCH,

    FORMAL_ARG,
    RETURN_SPEC,
    LLVM_CODE,
    CODE,

    RECORD,
    RECORD_BODY,
    RECORD_FIELD,
    VARIANT,
    INSTANCE,
    OVERLOAD,
    PROCEDURE,
    ENUMERATION,
    ENUM_MEMBER,
    GLOBAL_VARIABLE,
    EXTERNAL_PROCEDURE,
    EXTERNAL_ARG,
    EXTERNAL_VARIABLE,
    EVAL_TOPLEVEL,

    GLOBAL_ALIAS,

    IMPORT,
    MODULE_DECLARATION,
    MODULE_HOLDER,
    MODULE,

    ENV,

    PRIM_OP,

    TYPE,

    PATTERN,
    MULTI_PATTERN,

    VALUE_HOLDER,
    MULTI_STATIC,

    PVALUE,
    MULTI_PVALUE,

    EVALUE,
    MULTI_EVALUE,

    CVALUE,
    MULTI_CVALUE,

    DONT_CARE
};



//
// forwards
//

struct Source;
struct Location;

struct Token;

struct ANode;
struct Identifier;
struct DottedName;

struct Expr;
struct BoolLiteral;
struct IntLiteral;
struct FloatLiteral;
struct CharLiteral;
struct StringLiteral;
struct IdentifierLiteral;
struct NameRef;
struct FILEExpr;
struct LINEExpr;
struct COLUMNExpr;
struct ARGExpr;
struct Tuple;
struct Paren;
struct Indexing;
struct Call;
struct FieldRef;
struct StaticIndexing;
struct UnaryOp;
struct BinaryOp;
struct And;
struct Or;
struct IfExpr;
struct Lambda;
struct Unpack;
struct StaticExpr;
struct DispatchExpr;
struct ForeignExpr;
struct ObjectExpr;
struct EvalExpr;

struct ExprList;

struct Statement;
struct Block;
struct Label;
struct Binding;
struct Assignment;
struct InitAssignment;
struct UpdateAssignment;
struct Goto;
struct Switch;
struct CaseBlock;
struct Return;
struct If;
struct ExprStatement;
struct While;
struct Break;
struct Continue;
struct For;
struct ForeignStatement;
struct Try;
struct Catch;
struct Throw;
struct StaticFor;
struct Finally;
struct OnError;
struct Unreachable;
struct EvalStatement;

struct FormalArg;
struct ReturnSpec;
struct LLVMCode;
struct Code;

struct TopLevelItem;
struct Record;
struct RecordBody;
struct RecordField;
struct Variant;
struct Instance;
struct Overload;
struct Procedure;
struct Enumeration;
struct EnumMember;
struct GlobalVariable;
struct GVarInstance;
struct ExternalProcedure;
struct ExternalArg;
struct ExternalVariable;
struct EvalTopLevel;

struct GlobalAlias;

struct Import;
struct ImportModule;
struct ImportStar;
struct ImportMembers;
struct ModuleHolder;
struct Module;
struct ModuleDeclaration;

struct Env;

struct PrimOp;

struct Type;
struct BoolType;
struct IntegerType;
struct FloatType;
struct ComplexType;
struct ArrayType;
struct VecType;
struct TupleType;
struct UnionType;
struct PointerType;
struct CodePointerType;
struct CCodePointerType;
struct RecordType;
struct VariantType;
struct StaticType;
struct EnumType;

struct Pattern;
struct PatternCell;
struct PatternStruct;

struct MultiPattern;
struct MultiPatternCell;
struct MultiPatternList;

struct ValueHolder;
struct MultiStatic;

struct PValue;
struct MultiPValue;

struct EValue;
struct MultiEValue;

struct CValue;
struct MultiCValue;

struct ObjectTable;

struct ValueStackEntry;


//
// Pointer typedefs
//

typedef Pointer<Source> SourcePtr;
typedef Pointer<Location> LocationPtr;

typedef Pointer<Token> TokenPtr;

typedef Pointer<ANode> ANodePtr;
typedef Pointer<Identifier> IdentifierPtr;
typedef Pointer<DottedName> DottedNamePtr;

typedef Pointer<Expr> ExprPtr;
typedef Pointer<BoolLiteral> BoolLiteralPtr;
typedef Pointer<IntLiteral> IntLiteralPtr;
typedef Pointer<FloatLiteral> FloatLiteralPtr;
typedef Pointer<CharLiteral> CharLiteralPtr;
typedef Pointer<StringLiteral> StringLiteralPtr;
typedef Pointer<IdentifierLiteral> IdentifierLiteralPtr;
typedef Pointer<NameRef> NameRefPtr;
typedef Pointer<FILEExpr> FILEExprPtr;
typedef Pointer<LINEExpr> LINEExprPtr;
typedef Pointer<COLUMNExpr> COLUMNExprPtr;
typedef Pointer<ARGExpr> ARGExprPtr;
typedef Pointer<Tuple> TuplePtr;
typedef Pointer<Paren> ParenPtr;
typedef Pointer<Indexing> IndexingPtr;
typedef Pointer<Call> CallPtr;
typedef Pointer<FieldRef> FieldRefPtr;
typedef Pointer<StaticIndexing> StaticIndexingPtr;
typedef Pointer<UnaryOp> UnaryOpPtr;
typedef Pointer<BinaryOp> BinaryOpPtr;
typedef Pointer<And> AndPtr;
typedef Pointer<Or> OrPtr;
typedef Pointer<IfExpr> IfExprPtr;
typedef Pointer<Lambda> LambdaPtr;
typedef Pointer<Unpack> UnpackPtr;
typedef Pointer<StaticExpr> StaticExprPtr;
typedef Pointer<DispatchExpr> DispatchExprPtr;
typedef Pointer<ForeignExpr> ForeignExprPtr;
typedef Pointer<ObjectExpr> ObjectExprPtr;
typedef Pointer<EvalExpr> EvalExprPtr;

typedef Pointer<ExprList> ExprListPtr;

typedef Pointer<Statement> StatementPtr;
typedef Pointer<Block> BlockPtr;
typedef Pointer<Label> LabelPtr;
typedef Pointer<Binding> BindingPtr;
typedef Pointer<Assignment> AssignmentPtr;
typedef Pointer<InitAssignment> InitAssignmentPtr;
typedef Pointer<UpdateAssignment> UpdateAssignmentPtr;
typedef Pointer<Goto> GotoPtr;
typedef Pointer<Switch> SwitchPtr;
typedef Pointer<CaseBlock> CaseBlockPtr;
typedef Pointer<Return> ReturnPtr;
typedef Pointer<If> IfPtr;
typedef Pointer<ExprStatement> ExprStatementPtr;
typedef Pointer<While> WhilePtr;
typedef Pointer<Break> BreakPtr;
typedef Pointer<Continue> ContinuePtr;
typedef Pointer<For> ForPtr;
typedef Pointer<ForeignStatement> ForeignStatementPtr;
typedef Pointer<Try> TryPtr;
typedef Pointer<Catch> CatchPtr;
typedef Pointer<Throw> ThrowPtr;
typedef Pointer<StaticFor> StaticForPtr;
typedef Pointer<Finally> FinallyPtr;
typedef Pointer<OnError> OnErrorPtr;
typedef Pointer<Unreachable> UnreachablePtr;
typedef Pointer<EvalStatement> EvalStatementPtr;

typedef Pointer<FormalArg> FormalArgPtr;
typedef Pointer<ReturnSpec> ReturnSpecPtr;
typedef Pointer<LLVMCode> LLVMCodePtr;
typedef Pointer<Code> CodePtr;

typedef Pointer<TopLevelItem> TopLevelItemPtr;
typedef Pointer<Record> RecordPtr;
typedef Pointer<RecordBody> RecordBodyPtr;
typedef Pointer<RecordField> RecordFieldPtr;
typedef Pointer<Variant> VariantPtr;
typedef Pointer<Instance> InstancePtr;
typedef Pointer<Overload> OverloadPtr;
typedef Pointer<Procedure> ProcedurePtr;
typedef Pointer<Enumeration> EnumerationPtr;
typedef Pointer<EnumMember> EnumMemberPtr;
typedef Pointer<GlobalVariable> GlobalVariablePtr;
typedef Pointer<GVarInstance> GVarInstancePtr;
typedef Pointer<ExternalProcedure> ExternalProcedurePtr;
typedef Pointer<ExternalArg> ExternalArgPtr;
typedef Pointer<ExternalVariable> ExternalVariablePtr;
typedef Pointer<EvalTopLevel> EvalTopLevelPtr;

typedef Pointer<GlobalAlias> GlobalAliasPtr;

typedef Pointer<Import> ImportPtr;
typedef Pointer<ImportModule> ImportModulePtr;
typedef Pointer<ImportStar> ImportStarPtr;
typedef Pointer<ImportMembers> ImportMembersPtr;
typedef Pointer<ModuleHolder> ModuleHolderPtr;
typedef Pointer<Module> ModulePtr;
typedef Pointer<ModuleDeclaration> ModuleDeclarationPtr;

typedef Pointer<Env> EnvPtr;

typedef Pointer<PrimOp> PrimOpPtr;

typedef Pointer<Type> TypePtr;
typedef Pointer<BoolType> BoolTypePtr;
typedef Pointer<IntegerType> IntegerTypePtr;
typedef Pointer<FloatType> FloatTypePtr;
typedef Pointer<ComplexType> ComplexTypePtr;
typedef Pointer<ArrayType> ArrayTypePtr;
typedef Pointer<VecType> VecTypePtr;
typedef Pointer<TupleType> TupleTypePtr;
typedef Pointer<UnionType> UnionTypePtr;
typedef Pointer<PointerType> PointerTypePtr;
typedef Pointer<CodePointerType> CodePointerTypePtr;
typedef Pointer<CCodePointerType> CCodePointerTypePtr;
typedef Pointer<RecordType> RecordTypePtr;
typedef Pointer<VariantType> VariantTypePtr;
typedef Pointer<StaticType> StaticTypePtr;
typedef Pointer<EnumType> EnumTypePtr;

typedef Pointer<Pattern> PatternPtr;
typedef Pointer<PatternCell> PatternCellPtr;
typedef Pointer<PatternStruct> PatternStructPtr;
typedef Pointer<MultiPattern> MultiPatternPtr;
typedef Pointer<MultiPatternCell> MultiPatternCellPtr;
typedef Pointer<MultiPatternList> MultiPatternListPtr;

typedef Pointer<ValueHolder> ValueHolderPtr;
typedef Pointer<MultiStatic> MultiStaticPtr;

typedef Pointer<PValue> PValuePtr;
typedef Pointer<MultiPValue> MultiPValuePtr;

typedef Pointer<EValue> EValuePtr;
typedef Pointer<MultiEValue> MultiEValuePtr;

typedef Pointer<CValue> CValuePtr;
typedef Pointer<MultiCValue> MultiCValuePtr;

typedef Pointer<ObjectTable> ObjectTablePtr;



//
// Source, Location
//

struct Source : public Object {
    string fileName;
    char *data;
    int size;
    Source(const string &fileName, char *data, int size)
        : Object(SOURCE), fileName(fileName), data(data), size(size) {}
    ~Source() {
        delete [] data;
    }
};

struct Location : public Object {
    SourcePtr source;
    int offset;
    Location(const SourcePtr &source, int offset)
        : Object(LOCATION), source(source), offset(offset) {}
};



//
// error module
//

struct CompileContextEntry {
    ObjectPtr callable;
    bool hasParams;
    vector<ObjectPtr> params;
    vector<unsigned> dispatchIndices;

    LocationPtr location;

    CompileContextEntry(ObjectPtr callable)
        : callable(callable), hasParams(false) {}

    CompileContextEntry(ObjectPtr callable, const vector<ObjectPtr> &params)
        : callable(callable), hasParams(true), params(params) {}

    CompileContextEntry(ObjectPtr callable,
                        const vector<ObjectPtr> &params,
                        const vector<unsigned> &dispatchIndices)
        : callable(callable), hasParams(true), params(params), dispatchIndices(dispatchIndices) {}
};

void pushCompileContext(ObjectPtr obj);
void pushCompileContext(ObjectPtr obj, const vector<ObjectPtr> &params);
void pushCompileContext(ObjectPtr obj,
                        const vector<ObjectPtr> &params,
                        const vector<unsigned> &dispatchIndices);
void popCompileContext();
vector<CompileContextEntry> getCompileContext();
void setCompileContext(const vector<CompileContextEntry> &x);

struct CompileContextPusher {
    CompileContextPusher(ObjectPtr obj) {
        pushCompileContext(obj);
    }
    CompileContextPusher(ObjectPtr obj, const vector<ObjectPtr> &params) {
        pushCompileContext(obj, params);
    }
    CompileContextPusher(ObjectPtr obj, const vector<TypePtr> &params) {
        vector<ObjectPtr> params2;
        for (unsigned i = 0; i < params.size(); ++i)
            params2.push_back((Object *)(params[i].ptr()));
        pushCompileContext(obj, params2);
    }
    CompileContextPusher(ObjectPtr obj,
                         const vector<TypePtr> &params,
                         const vector<unsigned> &dispatchIndices) {
        vector<ObjectPtr> params2;
        for (unsigned i = 0; i < params.size(); ++i)
            params2.push_back((Object *)(params[i].ptr()));
        pushCompileContext(obj, params2, dispatchIndices);
    }
    ~CompileContextPusher() {
        popCompileContext();
    }
};

void pushLocation(LocationPtr location);
void popLocation();
LocationPtr topLocation();

struct LocationContext {
    LocationPtr loc;
    LocationContext(LocationPtr loc)
        : loc(loc) {
        if (loc.ptr()) pushLocation(loc);
    }
    ~LocationContext() {
        if (loc.ptr())
            popLocation();
    }
private :
    LocationContext(const LocationContext &) {}
    void operator=(const LocationContext &) {}
};

void setAbortOnError(bool flag);
void warning(const string &msg);
void error(const string &msg);
void fmtError(const char *fmt, ...);

template <class T>
void error(Pointer<T> context, const string &msg)
{
    if (context->location.ptr())
        pushLocation(context->location);
    error(msg);
}

void argumentError(unsigned int index, const string &msg);

void arityError(int expected, int received);
void arityError2(int minExpected, int received);

template <class T>
void arityError(Pointer<T> context, int expected, int received)
{
    if (context->location.ptr())
        pushLocation(context->location);
    arityError(expected, received);
}

template <class T>
void arityError2(Pointer<T> context, int minExpected, int received)
{
    if (context->location.ptr())
        pushLocation(context->location);
    arityError2(minExpected, received);
}

void ensureArity(MultiStaticPtr args, unsigned int size);
void ensureArity(MultiEValuePtr args, unsigned int size);
void ensureArity(MultiPValuePtr args, unsigned int size);
void ensureArity(MultiCValuePtr args, unsigned int size);

template <class T>
void ensureArity(const vector<T> &args, int size)
{
    if ((int)args.size() != size)
        arityError(size, args.size());
}

template <class T>
void ensureArity2(const vector<T> &args, int size, bool hasVarArgs)
{
    if (!hasVarArgs)
        ensureArity(args, size);
    else if ((int)args.size() < size)
        arityError2(size, args.size());
}

void arityMismatchError(int leftArity, int rightArity);

void typeError(const string &expected, TypePtr receivedType);
void typeError(TypePtr expectedType, TypePtr receivedType);

void argumentTypeError(unsigned int index,
                       const string &expected,
                       TypePtr receivedType);
void argumentTypeError(unsigned int index,
                       TypePtr expectedType,
                       TypePtr receivedType);

void indexRangeError(const string &kind,
                     size_t value,
                     size_t maxValue);

void argumentIndexRangeError(unsigned int index,
                             const string &kind,
                             size_t value,
                             size_t maxValue);

void computeLineCol(LocationPtr location,
                    int &line,
                    int &column,
                    int &tabColumn);

void printFileLineCol(ostream &out, LocationPtr location);

void invalidStaticObjectError(ObjectPtr obj);
void argumentInvalidStaticObjectError(unsigned int index, ObjectPtr obj);

struct DebugPrinter {
    static int indent;
    ObjectPtr obj;
    DebugPrinter(ObjectPtr obj);
    ~DebugPrinter();
};



//
// Token
//

enum TokenKind {
    T_SYMBOL,
    T_KEYWORD,
    T_IDENTIFIER,
    T_STRING_LITERAL,
    T_CHAR_LITERAL,
    T_INT_LITERAL,
    T_FLOAT_LITERAL,
    T_STATIC_INDEX,
    T_SPACE,
    T_LINE_COMMENT,
    T_BLOCK_COMMENT,
    T_LLVM
};

struct Token : public Object {
    LocationPtr location;
    int tokenKind;
    string str;
    Token(int tokenKind)
        : Object(TOKEN), tokenKind(tokenKind) {}
    Token(int tokenKind, const string &str)
        : Object(TOKEN), tokenKind(tokenKind), str(str) {}
};



//
// lexer module
//

void tokenize(SourcePtr source, vector<TokenPtr> &tokens);

void tokenize(SourcePtr source, int offset, int length,
              vector<TokenPtr> &tokens);



//
// AST
//

struct ANode : public Object {
    LocationPtr location;
    ANode(int objKind)
        : Object(objKind) {}
};

struct Identifier : public ANode {
    string str;
    Identifier(const string &str)
        : ANode(IDENTIFIER), str(str) {}
};

struct DottedName : public ANode {
    vector<IdentifierPtr> parts;
    DottedName()
        : ANode(DOTTED_NAME) {}
    DottedName(const vector<IdentifierPtr> &parts)
        : ANode(DOTTED_NAME), parts(parts) {}
};




//
// Expr
//

enum ExprKind {
    BOOL_LITERAL,
    INT_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,
    IDENTIFIER_LITERAL,

    FILE_EXPR,
    LINE_EXPR,
    COLUMN_EXPR,
    ARG_EXPR,

    NAME_REF,
    TUPLE,
    PAREN,
    INDEXING,
    CALL,
    FIELD_REF,
    STATIC_INDEXING,
    UNARY_OP,
    BINARY_OP,
    AND,
    OR,

    IF_EXPR,
    LAMBDA,
    UNPACK,
    STATIC_EXPR,
    DISPATCH_EXPR,

    FOREIGN_EXPR,
    OBJECT_EXPR,

    EVAL_EXPR
};

struct Expr : public ANode {
    ExprKind exprKind;
    LocationPtr startLocation;
    LocationPtr endLocation;

    MultiPValuePtr cachedAnalysis;

    Expr(ExprKind exprKind)
        : ANode(EXPRESSION), exprKind(exprKind) {}

    string asString() {
        assert(startLocation->source == endLocation->source);
        char *data = startLocation->source->data;
        return string(data + startLocation->offset, data + endLocation->offset);
    }
};

struct BoolLiteral : public Expr {
    bool value;
    BoolLiteral(bool value)
        : Expr(BOOL_LITERAL), value(value) {}
};

struct IntLiteral : public Expr {
    string value;
    string suffix;
    IntLiteral(const string &value)
        : Expr(INT_LITERAL), value(value) {}
    IntLiteral(const string &value, const string &suffix)
        : Expr(INT_LITERAL), value(value), suffix(suffix) {}
};

struct FloatLiteral : public Expr {
    string value;
    string suffix;
    FloatLiteral(const string &value)
        : Expr(FLOAT_LITERAL), value(value) {}
    FloatLiteral(const string &value, const string &suffix)
        : Expr(FLOAT_LITERAL), value(value), suffix(suffix) {}
};


struct CharLiteral : public Expr {
    char value;
    ExprPtr desugared;
    CharLiteral(char value)
        : Expr(CHAR_LITERAL), value(value) {}
};

struct StringLiteral : public Expr {
    string value;
    StringLiteral(const string &value)
        : Expr(STRING_LITERAL), value(value) {}
};

struct IdentifierLiteral : public Expr {
    IdentifierPtr value;
    IdentifierLiteral(IdentifierPtr value)
        : Expr(IDENTIFIER_LITERAL), value(value) {}
};

struct NameRef : public Expr {
    IdentifierPtr name;
    NameRef(IdentifierPtr name)
        : Expr(NAME_REF), name(name) {}
};

struct FILEExpr : public Expr {
    FILEExpr()
        : Expr(FILE_EXPR) {}
};

struct LINEExpr : public Expr {
    LINEExpr()
        : Expr(LINE_EXPR) {}
};

struct COLUMNExpr : public Expr {
    COLUMNExpr()
        : Expr(COLUMN_EXPR) {}
};

struct ARGExpr : public Expr {
    IdentifierPtr name;
    ARGExpr(IdentifierPtr name)
        : Expr(ARG_EXPR), name(name) {}
};

struct Tuple : public Expr {
    ExprListPtr args;
    Tuple(ExprListPtr args)
        : Expr(TUPLE), args(args) {}
};

struct Paren : public Expr {
    ExprListPtr args;
    Paren(ExprListPtr args)
        : Expr(PAREN), args(args) {}
};

struct Indexing : public Expr {
    ExprPtr expr;
    ExprListPtr args;
    Indexing(ExprPtr expr, ExprListPtr args)
        : Expr(INDEXING), expr(expr), args(args) {}
};

struct Call : public Expr {
    ExprPtr expr;
    ExprListPtr args;
    Call(ExprPtr expr, ExprListPtr args)
        : Expr(CALL), expr(expr), args(args) {}
};

struct FieldRef : public Expr {
    ExprPtr expr;
    IdentifierPtr name;
    ExprPtr desugared;
    FieldRef(ExprPtr expr, IdentifierPtr name)
        : Expr(FIELD_REF), expr(expr), name(name) {}
};

struct StaticIndexing : public Expr {
    ExprPtr expr;
    size_t index;
    ExprPtr desugared;
    StaticIndexing(ExprPtr expr, size_t index)
        : Expr(STATIC_INDEXING), expr(expr), index(index) {}
};

enum UnaryOpKind {
    DEREFERENCE,
    ADDRESS_OF,
    PLUS,
    MINUS,
    NOT
};

struct UnaryOp : public Expr {
    int op;
    ExprPtr expr;
    ExprPtr desugared;
    UnaryOp(int op, ExprPtr expr)
        : Expr(UNARY_OP), op(op), expr(expr) {}
};


enum BinaryOpKind {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    REMAINDER,
    EQUALS,
    NOT_EQUALS,
    LESSER,
    LESSER_EQUALS,
    GREATER,
    GREATER_EQUALS
};

struct BinaryOp : public Expr {
    int op;
    ExprPtr expr1, expr2;
    ExprPtr desugared;
    BinaryOp(int op, ExprPtr expr1, ExprPtr expr2)
        : Expr(BINARY_OP), op(op), expr1(expr1), expr2(expr2) {}
};

struct And : public Expr {
    ExprPtr expr1, expr2;
    And(ExprPtr expr1, ExprPtr expr2)
        : Expr(AND), expr1(expr1), expr2(expr2) {}
};

struct Or : public Expr {
    ExprPtr expr1, expr2;
    Or(ExprPtr expr1, ExprPtr expr2)
        : Expr(OR), expr1(expr1), expr2(expr2) {}
};

struct IfExpr : public Expr {
    ExprPtr condition;
    ExprPtr thenPart, elsePart;
    ExprPtr desugared;
    IfExpr(ExprPtr condition, ExprPtr thenPart, ExprPtr elsePart)
        : Expr(IF_EXPR), condition(condition), thenPart(thenPart),
          elsePart(elsePart) {}
};

struct Lambda : public Expr {
    bool captureByRef;
    vector<FormalArgPtr> formalArgs;
    FormalArgPtr formalVarArg;
    StatementPtr body;

    ExprPtr converted;

    bool initialized;
    vector<string> freeVars;

    // if freevars are present
    RecordPtr lambdaRecord;
    TypePtr lambdaType;

    // if freevars are absent
    ProcedurePtr lambdaProc;

    Lambda(bool captureByRef) :
        Expr(LAMBDA), captureByRef(captureByRef),
        initialized(false) {}
    Lambda(bool captureByRef,
           const vector<FormalArgPtr> &formalArgs,
           FormalArgPtr formalVarArg,
           StatementPtr body)
        : Expr(LAMBDA), captureByRef(captureByRef),
          formalArgs(formalArgs), formalVarArg(formalVarArg),
          body(body), initialized(false) {}
};

struct Unpack : public Expr {
    ExprPtr expr;
    Unpack(ExprPtr expr) :
        Expr(UNPACK), expr(expr) {}
};

struct StaticExpr : public Expr {
    ExprPtr expr;
    ExprPtr desugared;
    StaticExpr(ExprPtr expr) :
        Expr(STATIC_EXPR), expr(expr) {}
};

struct DispatchExpr : public Expr {
    ExprPtr expr;
    DispatchExpr(ExprPtr expr) :
        Expr(DISPATCH_EXPR), expr(expr) {}
};

struct ForeignExpr : public Expr {
    string moduleName;
    EnvPtr foreignEnv;
    ExprPtr expr;

    ForeignExpr(const string &moduleName, ExprPtr expr)
        : Expr(FOREIGN_EXPR), moduleName(moduleName), expr(expr) {}
    ForeignExpr(EnvPtr foreignEnv, ExprPtr expr)
        : Expr(FOREIGN_EXPR), foreignEnv(foreignEnv), expr(expr) {}

    ForeignExpr(const string &moduleName, EnvPtr foreignEnv, ExprPtr expr)
        : Expr(FOREIGN_EXPR), moduleName(moduleName),
          foreignEnv(foreignEnv), expr(expr) {}

    EnvPtr getEnv();
};

struct ObjectExpr : public Expr {
    ObjectPtr obj;
    ObjectExpr(ObjectPtr obj)
        : Expr(OBJECT_EXPR), obj(obj) {}
};

struct EvalExpr : public Expr {
    ExprPtr args;
    bool evaled;
    ExprListPtr value;

    EvalExpr(ExprPtr args)
        : Expr(EVAL_EXPR), args(args), evaled(false) {}
};



//
// ExprList
//

struct ExprList : public Object {
    vector<ExprPtr> exprs;

    MultiPValuePtr cachedAnalysis;

    ExprList()
        : Object(EXPR_LIST) {}
    ExprList(ExprPtr x)
        : Object(EXPR_LIST) {
        exprs.push_back(x);
    }
    ExprList(const vector<ExprPtr> &exprs)
        : Object(EXPR_LIST), exprs(exprs) {}
    unsigned size() { return exprs.size(); }
    void add(ExprPtr x) { exprs.push_back(x); }
    void add(ExprListPtr x) {
        exprs.insert(exprs.end(), x->exprs.begin(), x->exprs.end());
    }
};



//
// Stmt
//

enum StatementKind {
    BLOCK,
    LABEL,
    BINDING,
    ASSIGNMENT,
    INIT_ASSIGNMENT,
    UPDATE_ASSIGNMENT,
    GOTO,
    RETURN,
    IF,
    SWITCH,
    EXPR_STATEMENT,
    WHILE,
    BREAK,
    CONTINUE,
    FOR,
    FOREIGN_STATEMENT,
    TRY,
    THROW,
    STATIC_FOR,
    FINALLY,
    ONERROR,
    UNREACHABLE,
    EVAL_STATEMENT
};

struct Statement : public ANode {
    StatementKind stmtKind;
    Statement(StatementKind stmtKind)
        : ANode(STATEMENT), stmtKind(stmtKind) {}
};

struct Block : public Statement {
    vector<StatementPtr> statements;
    Block()
        : Statement(BLOCK) {}
    Block(const vector<StatementPtr> &statements)
        : Statement(BLOCK), statements(statements) {}
};

struct Label : public Statement {
    IdentifierPtr name;
    Label(IdentifierPtr name)
        : Statement(LABEL), name(name) {}
};

enum BindingKind {
    VAR,
    REF,
    ALIAS,
    FORWARD
};

struct Binding : public Statement {
    int bindingKind;
    vector<IdentifierPtr> names;
    ExprListPtr values;
    Binding(int bindingKind,
            const vector<IdentifierPtr> &names,
            ExprListPtr values)
        : Statement(BINDING), bindingKind(bindingKind),
          names(names), values(values) {}
};

struct Assignment : public Statement {
    ExprListPtr left, right;
    Assignment(ExprListPtr left,
               ExprListPtr right)
        : Statement(ASSIGNMENT), left(left), right(right) {}
};

struct InitAssignment : public Statement {
    ExprListPtr left, right;
    InitAssignment(ExprListPtr left,
                   ExprListPtr right)
        : Statement(INIT_ASSIGNMENT), left(left), right(right) {}
    InitAssignment(ExprPtr leftExpr, ExprPtr rightExpr)
        : Statement(INIT_ASSIGNMENT),
          left(new ExprList(leftExpr)),
          right(new ExprList(rightExpr)) {}
};

enum UpdateOpKind {
    UPDATE_ADD,
    UPDATE_SUBTRACT,
    UPDATE_MULTIPLY,
    UPDATE_DIVIDE,
    UPDATE_REMAINDER
};

struct UpdateAssignment : public Statement {
    int op;
    ExprPtr left, right;
    UpdateAssignment(int op, ExprPtr left, ExprPtr right)
        : Statement(UPDATE_ASSIGNMENT), op(op), left(left),
          right(right) {}
};

struct Goto : public Statement {
    IdentifierPtr labelName;
    Goto(IdentifierPtr labelName)
        : Statement(GOTO), labelName(labelName) {}
};

enum ReturnKind {
    RETURN_VALUE,
    RETURN_REF,
    RETURN_FORWARD
};

struct Return : public Statement {
    ReturnKind returnKind;
    ExprListPtr values;
    Return(ReturnKind returnKind, ExprListPtr values)
        : Statement(RETURN), returnKind(returnKind), values(values) {}
};

struct If : public Statement {
    ExprPtr condition;
    StatementPtr thenPart, elsePart;
    If(ExprPtr condition, StatementPtr thenPart)
        : Statement(IF), condition(condition), thenPart(thenPart) {}
    If(ExprPtr condition, StatementPtr thenPart, StatementPtr elsePart)
        : Statement(IF), condition(condition), thenPart(thenPart),
          elsePart(elsePart) {}
};

struct Switch : public Statement {
    ExprPtr expr;
    vector<CaseBlockPtr> caseBlocks;
    StatementPtr defaultCase;

    StatementPtr desugared;

    Switch(ExprPtr expr,
           const vector<CaseBlockPtr> &caseBlocks,
           StatementPtr defaultCase)
        : Statement(SWITCH), expr(expr), caseBlocks(caseBlocks),
          defaultCase(defaultCase) {}
};

struct CaseBlock : public ANode {
    ExprListPtr caseLabels;
    StatementPtr body;
    CaseBlock(ExprListPtr caseLabels, StatementPtr body)
        : ANode(CASE_BLOCK), caseLabels(caseLabels), body(body) {}
};

struct ExprStatement : public Statement {
    ExprPtr expr;
    ExprStatement(ExprPtr expr)
        : Statement(EXPR_STATEMENT), expr(expr) {}
};

struct While : public Statement {
    ExprPtr condition;
    StatementPtr body;
    While(ExprPtr condition, StatementPtr body)
        : Statement(WHILE), condition(condition), body(body) {}
};

struct Break : public Statement {
    Break()
        : Statement(BREAK) {}
};

struct Continue : public Statement {
    Continue()
        : Statement(CONTINUE) {}
};

struct For : public Statement {
    vector<IdentifierPtr> variables;
    ExprPtr expr;
    StatementPtr body;
    StatementPtr desugared;
    For(const vector<IdentifierPtr> &variables,
        ExprPtr expr,
        StatementPtr body)
        : Statement(FOR), variables(variables), expr(expr), body(body) {}
};

struct ForeignStatement : public Statement {
    string moduleName;
    EnvPtr foreignEnv;
    StatementPtr statement;
    ForeignStatement(const string &moduleName, StatementPtr statement)
        : Statement(FOREIGN_STATEMENT), moduleName(moduleName),
          statement(statement) {}
    ForeignStatement(EnvPtr foreignEnv, StatementPtr statement)
        : Statement(FOREIGN_STATEMENT), foreignEnv(foreignEnv),
          statement(statement) {}
    ForeignStatement(const string &moduleName, EnvPtr foreignEnv,
                     StatementPtr statement)
        : Statement(FOREIGN_STATEMENT), moduleName(moduleName),
          foreignEnv(foreignEnv), statement(statement) {}

    EnvPtr getEnv();
};

struct Try : public Statement {
    StatementPtr tryBlock;
    vector<CatchPtr> catchBlocks;
    StatementPtr desugaredCatchBlock;

    Try(StatementPtr tryBlock,
        vector<CatchPtr> catchBlocks)
        : Statement(TRY), tryBlock(tryBlock),
          catchBlocks(catchBlocks) {}
};

struct Catch : public ANode {
    IdentifierPtr exceptionVar;
    ExprPtr exceptionType; // optional
    StatementPtr body;

    Catch(IdentifierPtr exceptionVar,
          ExprPtr exceptionType,
          StatementPtr body)
        : ANode(CATCH), exceptionVar(exceptionVar),
          exceptionType(exceptionType), body(body) {}
};

struct Throw : public Statement {
    ExprPtr expr;
    Throw(ExprPtr expr)
        : Statement(THROW), expr(expr) {}
};

struct StaticFor : public Statement {
    IdentifierPtr variable;
    ExprListPtr values;
    StatementPtr body;

    bool clonesInitialized;
    vector<StatementPtr> clonedBodies;

    StaticFor(IdentifierPtr variable,
              ExprListPtr values,
              StatementPtr body)
        : Statement(STATIC_FOR), variable(variable), values(values),
          body(body), clonesInitialized(false) {}
};

struct Finally : public Statement {
    StatementPtr body;

    explicit Finally(StatementPtr body) : Statement(FINALLY), body(body) {}
};

struct OnError : public Statement {
    StatementPtr body;

    explicit OnError(StatementPtr body) : Statement(ONERROR), body(body) {}
};

struct Unreachable : public Statement {
    Unreachable()
        : Statement(UNREACHABLE) {}
};

struct EvalStatement : public Statement {
    ExprListPtr args;
    bool evaled;
    vector<StatementPtr> value;

    EvalStatement(ExprListPtr args)
        : Statement(EVAL_STATEMENT), args(args), evaled(false) {}
};



//
// Code
//

enum ValueTempness {
    TEMPNESS_DONTCARE,
    TEMPNESS_LVALUE,
    TEMPNESS_RVALUE,
    TEMPNESS_FORWARD
};

struct FormalArg : public ANode {
    IdentifierPtr name;
    ExprPtr type;
    ValueTempness tempness;
    FormalArg(IdentifierPtr name, ExprPtr type)
        : ANode(FORMAL_ARG), name(name), type(type),
          tempness(TEMPNESS_DONTCARE) {}
    FormalArg(IdentifierPtr name, ExprPtr type, ValueTempness tempness)
        : ANode(FORMAL_ARG), name(name), type(type),
          tempness(tempness) {}
};

struct ReturnSpec : public ANode {
    ExprPtr type;
    IdentifierPtr name;
    ReturnSpec(ExprPtr type)
        : ANode(RETURN_SPEC), type(type) {}
    ReturnSpec(ExprPtr type, IdentifierPtr name)
        : ANode(RETURN_SPEC), type(type), name(name) {}
};

struct PatternVar {
    bool isMulti;
    IdentifierPtr name;
    PatternVar(bool isMulti, IdentifierPtr name)
        : isMulti(isMulti), name(name) {}
    PatternVar()
        : isMulti(false) {}
};

struct LLVMCode : ANode {
    string body;
    LLVMCode(const string &body)
        : ANode(LLVM_CODE), body(body) {}
};

struct Code : public ANode {
    vector<PatternVar> patternVars;
    ExprPtr predicate;
    vector<FormalArgPtr> formalArgs;
    FormalArgPtr formalVarArg;
    bool returnSpecsDeclared;
    vector<ReturnSpecPtr> returnSpecs;
    ReturnSpecPtr varReturnSpec;
    StatementPtr body;
    LLVMCodePtr llvmBody;

    Code()
        : ANode(CODE), returnSpecsDeclared(false) {}
    Code(const vector<PatternVar> &patternVars,
         ExprPtr predicate,
         const vector<FormalArgPtr> &formalArgs,
         FormalArgPtr formalVarArg,
         const vector<ReturnSpecPtr> &returnSpecs,
         ReturnSpecPtr varReturnSpec,
         StatementPtr body)
        : ANode(CODE), patternVars(patternVars), predicate(predicate),
          formalArgs(formalArgs), formalVarArg(formalVarArg),
          returnSpecsDeclared(false),
          returnSpecs(returnSpecs), varReturnSpec(varReturnSpec),
          body(body) {}

    bool hasReturnSpecs() {
        return returnSpecsDeclared;
    }

    bool isLLVMBody() {
        return llvmBody.ptr() != NULL;
    }
    bool hasBody() {
        return body.ptr() || isLLVMBody();
    }
};



//
// Visibility
//

enum Visibility {
    PUBLIC,
    PRIVATE
};



//
// TopLevelItem
//

struct TopLevelItem : public ANode {
    EnvPtr env;
    IdentifierPtr name; // for named top level items
    Visibility visibility; // valid only if name != NULL
    TopLevelItem(int objKind)
        : ANode(objKind) {}
    TopLevelItem(int objKind, Visibility visibility)
        : ANode(objKind), visibility(visibility) {}
    TopLevelItem(int objKind, IdentifierPtr name, Visibility visibility)
        : ANode(objKind), name(name), visibility(visibility) {}
};

struct Record : public TopLevelItem {
    vector<PatternVar> patternVars;
    ExprPtr predicate;

    vector<IdentifierPtr> params;
    IdentifierPtr varParam;

    RecordBodyPtr body;

    vector<OverloadPtr> overloads;
    bool builtinOverloadInitialized;

    Record(Visibility visibility)
        : TopLevelItem(RECORD, visibility),
          builtinOverloadInitialized(false) {}
    Record(Visibility visibility,
           const vector<PatternVar> &patternVars, ExprPtr predicate)
        : TopLevelItem(RECORD, visibility),
          patternVars(patternVars),
          predicate(predicate),
          builtinOverloadInitialized(false) {}
    Record(IdentifierPtr name,
           Visibility visibility,
           const vector<PatternVar> &patternVars,
           ExprPtr predicate,
           const vector<IdentifierPtr> &params,
           IdentifierPtr varParam,
           RecordBodyPtr body)
        : TopLevelItem(RECORD, name, visibility),
          patternVars(patternVars), predicate(predicate),
          params(params), varParam(varParam), body(body),
          builtinOverloadInitialized(false) {}
};

struct RecordBody : public ANode {
    bool isComputed;
    ExprListPtr computed; // valid if isComputed == true
    vector<RecordFieldPtr> fields; // valid if isComputed == false
    RecordBody(ExprListPtr computed)
        : ANode(RECORD_BODY), isComputed(true), computed(computed) {}
    RecordBody(const vector<RecordFieldPtr> &fields)
        : ANode(RECORD_BODY), isComputed(false), fields(fields) {}
};

struct RecordField : public ANode {
    IdentifierPtr name;
    ExprPtr type;
    RecordField(IdentifierPtr name, ExprPtr type)
        : ANode(RECORD_FIELD), name(name), type(type) {}
};

struct Variant : public TopLevelItem {
    vector<PatternVar> patternVars;
    ExprPtr predicate;

    vector<IdentifierPtr> params;
    IdentifierPtr varParam;

    ExprListPtr defaultInstances;

    vector<InstancePtr> instances;

    vector<OverloadPtr> overloads;

    Variant(IdentifierPtr name,
            Visibility visibility,
            const vector<PatternVar> &patternVars,
            ExprPtr predicate,
            const vector<IdentifierPtr> &params,
            IdentifierPtr varParam,
            ExprListPtr defaultInstances)
        : TopLevelItem(VARIANT, name, visibility),
          patternVars(patternVars), predicate(predicate),
          params(params), varParam(varParam),
          defaultInstances(defaultInstances) {}
};

struct Instance : public TopLevelItem {
    vector<PatternVar> patternVars;
    ExprPtr predicate;
    ExprPtr target;
    ExprListPtr members;

    Instance(const vector<PatternVar> &patternVars,
             ExprPtr predicate,
             ExprPtr target,
             ExprListPtr members)
        : TopLevelItem(INSTANCE), patternVars(patternVars),
          predicate(predicate), target(target), members(members)
        {}
};

struct Overload : public TopLevelItem {
    ExprPtr target;
    CodePtr code;
    bool callByName;
    bool isInline;

    // pre-computed patterns for matchInvoke
    int patternsInitializedState; // 0:notinit, -1:initing, +1:inited
    vector<PatternCellPtr> cells;
    vector<MultiPatternCellPtr> multiCells;
    EnvPtr patternEnv;
    PatternPtr callablePattern;
    vector<PatternPtr> argPatterns;
    MultiPatternPtr varArgPattern;

    Overload(ExprPtr target,
             CodePtr code,
             bool callByName,
             bool isInline)
        : TopLevelItem(OVERLOAD), target(target), code(code),
          callByName(callByName), isInline(isInline),
          patternsInitializedState(0) {}
};

struct Procedure : public TopLevelItem {
    OverloadPtr interface;
    vector<OverloadPtr> overloads;
    ObjectTablePtr evaluatorCache; // HACK: used only for predicates

    Procedure(IdentifierPtr name, Visibility visibility)
        : TopLevelItem(PROCEDURE, name, visibility) {}

    Procedure(IdentifierPtr name, Visibility visibility, OverloadPtr interface)
        : TopLevelItem(PROCEDURE, name, visibility), interface(interface) {}
};

struct Enumeration : public TopLevelItem {
    vector<PatternVar> patternVars;
    ExprPtr predicate;

    vector<EnumMemberPtr> members;
    TypePtr type;
    Enumeration(IdentifierPtr name, Visibility visibility,
        const vector<PatternVar> &patternVars, ExprPtr predicate)
        : TopLevelItem(ENUMERATION, name, visibility),
          patternVars(patternVars), predicate(predicate) {}
    Enumeration(IdentifierPtr name, Visibility visibility,
                const vector<PatternVar> &patternVars, ExprPtr predicate,
                const vector<EnumMemberPtr> &members)
        : TopLevelItem(ENUMERATION, name, visibility),
          patternVars(patternVars), predicate(predicate),
          members(members) {}
};

struct EnumMember : public ANode {
    IdentifierPtr name;
    int index;
    TypePtr type;
    EnumMember(IdentifierPtr name)
        : ANode(ENUM_MEMBER), name(name), index(-1) {}
};

struct GlobalVariable : public TopLevelItem {
    vector<PatternVar> patternVars;
    ExprPtr predicate;

    vector<IdentifierPtr> params;
    IdentifierPtr varParam;
    ExprPtr expr;

    ObjectTablePtr instances;

    GlobalVariable(IdentifierPtr name,
                   Visibility visibility,
                   const vector<PatternVar> &patternVars,
                   ExprPtr predicate,
                   const vector<IdentifierPtr> &params,
                   IdentifierPtr varParam,
                   ExprPtr expr)
        : TopLevelItem(GLOBAL_VARIABLE, name, visibility),
          patternVars(patternVars), predicate(predicate),
          params(params), varParam(varParam), expr(expr) {}

    bool hasParams() const {
        return !params.empty() || (varParam.ptr() != NULL);
    }
};

struct GVarInstance : public Object {
    GlobalVariablePtr gvar;
    vector<ObjectPtr> params;

    bool analyzing;
    ExprPtr expr;
    EnvPtr env;
    MultiPValuePtr analysis;
    TypePtr type;
    llvm::GlobalVariable *llGlobal;

    GVarInstance(GlobalVariablePtr gvar,
                 const vector<ObjectPtr> &params)
        : Object(DONT_CARE), gvar(gvar), params(params),
          analyzing(false), llGlobal(NULL) {}
};

enum CallingConv {
    CC_DEFAULT,
    CC_STDCALL,
    CC_FASTCALL,
    CC_LLVM
};

struct ExternalProcedure : public TopLevelItem {
    vector<ExternalArgPtr> args;
    bool hasVarArgs;
    ExprPtr returnType;
    StatementPtr body;
    ExprListPtr attributes;

    bool attributesVerified;
    bool attrDLLImport;
    bool attrDLLExport;
    CallingConv callingConv;
    string attrAsmLabel;

    bool analyzed;
    bool bodyCodegenned;
    TypePtr returnType2;
    TypePtr ptrType;

    llvm::Function *llvmFunc;

    ExternalProcedure(Visibility visibility)
        : TopLevelItem(EXTERNAL_PROCEDURE, visibility), hasVarArgs(false),
          attributes(new ExprList()), attributesVerified(false),
          analyzed(false), llvmFunc(NULL) {}
    ExternalProcedure(IdentifierPtr name,
                      Visibility visibility,
                      const vector<ExternalArgPtr> &args,
                      bool hasVarArgs,
                      ExprPtr returnType,
                      StatementPtr body,
                      ExprListPtr attributes)
        : TopLevelItem(EXTERNAL_PROCEDURE, name, visibility), args(args),
          hasVarArgs(hasVarArgs), returnType(returnType), body(body),
          attributes(attributes), attributesVerified(false),
          analyzed(false), bodyCodegenned(false), llvmFunc(NULL) {}
};

struct ExternalArg : public ANode {
    IdentifierPtr name;
    ExprPtr type;
    TypePtr type2;
    ExternalArg(IdentifierPtr name, ExprPtr type)
        : ANode(EXTERNAL_ARG), name(name), type(type) {}
};

struct ExternalVariable : public TopLevelItem {
    ExprPtr type;
    ExprListPtr attributes;

    bool attributesVerified;
    bool attrDLLImport;
    bool attrDLLExport;

    TypePtr type2;
    llvm::GlobalVariable *llGlobal;

    ExternalVariable(Visibility visibility)
        : TopLevelItem(EXTERNAL_VARIABLE, visibility),
          attributes(new ExprList()), attributesVerified(false),
          llGlobal(NULL) {}
    ExternalVariable(IdentifierPtr name,
                     Visibility visibility,
                     ExprPtr type,
                     ExprListPtr attributes)
        : TopLevelItem(EXTERNAL_VARIABLE, name, visibility),
          type(type), attributes(attributes),
          attributesVerified(false), llGlobal(NULL) {}
};

struct EvalTopLevel : public TopLevelItem {
    ExprListPtr args;
    bool evaled;
    vector<TopLevelItemPtr> value;

    EvalTopLevel(ExprListPtr args)
        : TopLevelItem(EVAL_TOPLEVEL), args(args), evaled(false)
        {}
};


//
// GlobalAlias
//

struct GlobalAlias : public TopLevelItem {
    vector<PatternVar> patternVars;
    ExprPtr predicate;

    vector<IdentifierPtr> params;
    IdentifierPtr varParam;
    ExprPtr expr;

    GlobalAlias(IdentifierPtr name,
                Visibility visibility,
                const vector<PatternVar> &patternVars,
                ExprPtr predicate,
                const vector<IdentifierPtr> &params,
                IdentifierPtr varParam,
                ExprPtr expr)
        : TopLevelItem(GLOBAL_ALIAS, name, visibility),
          patternVars(patternVars), predicate(predicate),
          params(params), varParam(varParam), expr(expr) {}
    bool hasParams() const {
        return !params.empty() || (varParam.ptr() != NULL);
    }
};



//
// Imports
//

enum ImportKind {
    IMPORT_MODULE,
    IMPORT_STAR,
    IMPORT_MEMBERS
};

struct Import : public ANode {
    int importKind;
    DottedNamePtr dottedName;
    Visibility visibility;
    ModulePtr module;
    Import(int importKind, DottedNamePtr dottedName, Visibility visibility)
        : ANode(IMPORT), importKind(importKind), dottedName(dottedName),
          visibility(visibility) {}
};

struct ImportModule : public Import {
    IdentifierPtr alias;
    ImportModule(DottedNamePtr dottedName,
                 Visibility visibility,
                 IdentifierPtr alias)
        : Import(IMPORT_MODULE, dottedName, visibility), alias(alias) {}
};

struct ImportStar : public Import {
    ImportStar(DottedNamePtr dottedName, Visibility visibility)
        : Import(IMPORT_STAR, dottedName, visibility) {}
};

struct ImportedMember {
    Visibility visibility;
    IdentifierPtr name;
    IdentifierPtr alias;
    ImportedMember() {}
    ImportedMember(Visibility visibility, IdentifierPtr name, IdentifierPtr alias)
        : visibility(visibility), name(name), alias(alias) {}
};

struct ImportMembers : public Import {
    vector<ImportedMember> members;
    map<string, IdentifierPtr> aliasMap;
    ImportMembers(DottedNamePtr dottedName, Visibility visibility)
        : Import(IMPORT_MEMBERS, dottedName, visibility) {}
};


//
// Module
//

struct ModuleHolder : public Object {
    ImportModulePtr import;
    map<string, ModuleHolderPtr> children;
    ModuleHolder()
        : Object(MODULE_HOLDER) {}
};

struct ModuleDeclaration : public ANode {
    DottedNamePtr name;
    ExprListPtr attributes;

    ModuleDeclaration(DottedNamePtr name, ExprListPtr attributes)
        : ANode(MODULE_DECLARATION), name(name), attributes(attributes)
        {}
};

struct Module : public ANode {
    string moduleName;
    vector<ImportPtr> imports;
    ModuleDeclarationPtr declaration;
    LLVMCodePtr topLevelLLVM;
    vector<TopLevelItemPtr> topLevelItems;

    ModuleHolderPtr rootHolder;
    map<string, ObjectPtr> globals;

    ModuleHolderPtr publicRootHolder;
    map<string, ObjectPtr> publicGlobals;

    EnvPtr env;
    bool initialized;

    bool attributesVerified;
    vector<string> attrBuildFlags;
    IntegerTypePtr attrDefaultIntegerType;
    FloatTypePtr attrDefaultFloatType;

    map<string, set<ObjectPtr> > publicSymbols;
    bool publicSymbolsLoaded;
    int publicSymbolsLoading;

    map<string, set<ObjectPtr> > allSymbols;
    bool allSymbolsLoaded;
    int allSymbolsLoading;

    bool topLevelLLVMGenerated;
    bool externalsGenerated;

    Module(const string &moduleName)
        : ANode(MODULE), moduleName(moduleName),
          initialized(false),
          attributesVerified(false),
          publicSymbolsLoaded(false), publicSymbolsLoading(0),
          allSymbolsLoaded(false), allSymbolsLoading(0),
          topLevelLLVMGenerated(false),
          externalsGenerated(false) {}
    Module(const string &moduleName,
           const vector<ImportPtr> &imports,
           ModuleDeclarationPtr declaration,
           LLVMCodePtr topLevelLLVM,
           const vector<TopLevelItemPtr> &topLevelItems)
        : ANode(MODULE), moduleName(moduleName), imports(imports),
          declaration(declaration),
          topLevelLLVM(topLevelLLVM), topLevelItems(topLevelItems),
          initialized(false),
          attributesVerified(false),
          publicSymbolsLoaded(false), publicSymbolsLoading(0),
          allSymbolsLoaded(false), allSymbolsLoading(0),
          topLevelLLVMGenerated(false),
          externalsGenerated(false) {}
};



//
// parser module
//

ModulePtr parse(const string &moduleName, SourcePtr source);
ExprPtr parseExpr(SourcePtr source, int offset, int length);
ExprListPtr parseExprList(SourcePtr source, int offset, int length);
void parseStatements(SourcePtr source, int offset, int length,
    vector<StatementPtr> &statements);
void parseTopLevelItems(SourcePtr source, int offset, int length,
    vector<TopLevelItemPtr> &topLevels);



//
// printer module
//

ostream &operator<<(ostream &out, const Object &obj);

ostream &operator<<(ostream &out, const Object *obj);

template <class T>
ostream &operator<<(ostream &out, const Pointer<T> &p)
{
    out << *p;
    return out;
}

template <class T>
ostream &operator<<(ostream &out, const vector<T> &v)
{
    out << "[";
    typename vector<T>::const_iterator i, end;
    bool first = true;
    for (i = v.begin(), end = v.end(); i != end; ++i) {
        if (!first)
            out << ", ";
        first = false;
        out << *i;
    }
    out << "]";
    return out;
}

ostream &operator<<(ostream &out, const PatternVar &pvar);

void enableSafePrintName();
void disableSafePrintName();

struct SafePrintNameEnabler {
    SafePrintNameEnabler() { enableSafePrintName(); }
    ~SafePrintNameEnabler() { disableSafePrintName(); }
};

void printNameList(ostream &out, const vector<ObjectPtr> &x);
void printNameList(ostream &out, const vector<ObjectPtr> &x, const vector<unsigned> &dispatchIndices);
void printNameList(ostream &out, const vector<TypePtr> &x);
void printStaticName(ostream &out, ObjectPtr x);
void printName(ostream &out, ObjectPtr x);
void printTypeAndValue(ostream &out, EValuePtr ev);
void printValue(ostream &out, EValuePtr ev);



//
// clone ast
//

CodePtr clone(CodePtr x);
void clone(const vector<PatternVar> &x, vector<PatternVar> &out);
void clone(const vector<IdentifierPtr> &x, vector<IdentifierPtr> &out);
ExprPtr clone(ExprPtr x);
ExprPtr cloneOpt(ExprPtr x);
ExprListPtr clone(ExprListPtr x);
void clone(const vector<FormalArgPtr> &x, vector<FormalArgPtr> &out);
FormalArgPtr clone(FormalArgPtr x);
FormalArgPtr cloneOpt(FormalArgPtr x);
void clone(const vector<ReturnSpecPtr> &x, vector<ReturnSpecPtr> &out);
ReturnSpecPtr clone(ReturnSpecPtr x);
ReturnSpecPtr cloneOpt(ReturnSpecPtr x);
StatementPtr clone(StatementPtr x);
StatementPtr cloneOpt(StatementPtr x);
void clone(const vector<StatementPtr> &x, vector<StatementPtr> &out);
CaseBlockPtr clone(CaseBlockPtr x);
void clone(const vector<CaseBlockPtr> &x, vector<CaseBlockPtr> &out);
CatchPtr clone(CatchPtr x);
void clone(const vector<CatchPtr> &x, vector<CatchPtr> &out);



//
// Env
//

struct Env : public Object {
    ObjectPtr parent;
    ExprPtr callByNameExprHead;
    map<string, ObjectPtr> entries;
    Env()
        : Object(ENV) {}
    Env(ModulePtr parent)
        : Object(ENV), parent(parent.ptr()) {}
    Env(EnvPtr parent)
        : Object(ENV), parent(parent.ptr()) {}
};



//
// env module
//

void addGlobal(ModulePtr module,
               IdentifierPtr name,
               Visibility visibility,
               ObjectPtr value);
ObjectPtr lookupModuleHolder(ModuleHolderPtr mh, IdentifierPtr name);
ObjectPtr safeLookupModuleHolder(ModuleHolderPtr mh, IdentifierPtr name);
ObjectPtr lookupPrivate(ModulePtr module, IdentifierPtr name);
ObjectPtr lookupPublic(ModulePtr module, IdentifierPtr name);
ObjectPtr safeLookupPublic(ModulePtr module, IdentifierPtr name);

void addLocal(EnvPtr env, IdentifierPtr name, ObjectPtr value);
ObjectPtr lookupEnv(EnvPtr env, IdentifierPtr name);
ObjectPtr safeLookupEnv(EnvPtr env, IdentifierPtr name);
ModulePtr safeLookupModule(EnvPtr env);

ObjectPtr lookupEnvEx(EnvPtr env, IdentifierPtr name,
                      EnvPtr nonLocalEnv, bool &isNonLocal,
                      bool &isGlobal);

ExprPtr foreignExpr(EnvPtr env, ExprPtr expr);

ExprPtr lookupCallByNameExprHead(EnvPtr env);
LocationPtr safeLookupCallByNameLocation(EnvPtr env);



//
// loader module
//

extern map<string, string> globalFlags;

void addSearchPath(const string &path);
ModulePtr loadProgram(const string &fileName);
ModulePtr loadProgramSource(const string &name, const string &source);
ModulePtr loadedModule(const string &module);
const string &primOpName(PrimOpPtr x);
ModulePtr preludeModule();
ModulePtr primitivesModule();
ModulePtr staticModule(ObjectPtr x);



//
// PrimOp
//

enum PrimOpCode {
    PRIM_TypeP,
    PRIM_TypeSize,
    PRIM_TypeAlignment,
    PRIM_CallDefinedP,

    PRIM_primitiveCopy,

    PRIM_boolNot,

    PRIM_numericEqualsP,
    PRIM_numericLesserP,
    PRIM_numericAdd,
    PRIM_numericSubtract,
    PRIM_numericMultiply,
    PRIM_numericDivide,
    PRIM_numericNegate,

    PRIM_integerRemainder,
    PRIM_integerShiftLeft,
    PRIM_integerShiftRight,
    PRIM_integerBitwiseAnd,
    PRIM_integerBitwiseOr,
    PRIM_integerBitwiseXor,
    PRIM_integerBitwiseNot,

    PRIM_numericConvert,

    PRIM_integerAddChecked,
    PRIM_integerSubtractChecked,
    PRIM_integerMultiplyChecked,
    PRIM_integerDivideChecked,
    PRIM_integerRemainderChecked,
    PRIM_integerNegateChecked,
    PRIM_integerConvertChecked,

    PRIM_Pointer,

    PRIM_addressOf,
    PRIM_pointerDereference,
    PRIM_pointerEqualsP,
    PRIM_pointerLesserP,
    PRIM_pointerOffset,
    PRIM_pointerToInt,
    PRIM_intToPointer,

    PRIM_CodePointer,
    PRIM_makeCodePointer,

    PRIM_AttributeStdCall,
    PRIM_AttributeFastCall,
    PRIM_AttributeCCall,
    PRIM_AttributeDLLImport,
    PRIM_AttributeDLLExport,

    PRIM_CCodePointerP,
    PRIM_CCodePointer,
    PRIM_VarArgsCCodePointer,
    PRIM_StdCallCodePointer,
    PRIM_FastCallCodePointer,
    PRIM_makeCCodePointer,
    PRIM_callCCodePointer,

    PRIM_pointerCast,

    PRIM_Array,
    PRIM_arrayRef,
    PRIM_arrayElements,

    PRIM_Vec,

    PRIM_Tuple,
    PRIM_TupleElementCount,
    PRIM_tupleRef,
    PRIM_tupleElements,

    PRIM_Union,
    PRIM_UnionMemberCount,

    PRIM_RecordP,
    PRIM_RecordFieldCount,
    PRIM_RecordFieldName,
    PRIM_RecordWithFieldP,
    PRIM_recordFieldRef,
    PRIM_recordFieldRefByName,
    PRIM_recordFields,

    PRIM_VariantP,
    PRIM_VariantMemberIndex,
    PRIM_VariantMemberCount,
    PRIM_variantRepr,

    PRIM_Static,
    PRIM_ModuleName,
    PRIM_StaticName,
    PRIM_staticIntegers,
    PRIM_staticFieldRef,

    PRIM_EnumP,
    PRIM_EnumMemberCount,
    PRIM_EnumMemberName,
    PRIM_enumToInt,
    PRIM_intToEnum,

    PRIM_IdentifierP,
    PRIM_IdentifierSize,
    PRIM_IdentifierConcat,
    PRIM_IdentifierSlice,
    PRIM_IdentifierModuleName,
    PRIM_IdentifierStaticName,

    PRIM_FlagP,
    PRIM_Flag,

    PRIM_atomicFence,
    PRIM_atomicRMW,
    PRIM_atomicLoad,
    PRIM_atomicStore,
    PRIM_atomicCompareExchange,

    PRIM_OrderUnordered,
    PRIM_OrderMonotonic,
    PRIM_OrderAcquire,
    PRIM_OrderRelease,
    PRIM_OrderAcqRel,
    PRIM_OrderSeqCst,

    PRIM_RMWXchg,
    PRIM_RMWAdd,
    PRIM_RMWSubtract,
    PRIM_RMWAnd,
    PRIM_RMWNAnd,
    PRIM_RMWOr,
    PRIM_RMWXor,
    PRIM_RMWMin,
    PRIM_RMWMax,
    PRIM_RMWUMin,
    PRIM_RMWUMax
};

struct PrimOp : public Object {
    int primOpCode;
    PrimOp(int primOpCode)
        : Object(PRIM_OP), primOpCode(primOpCode) {}
};



//
// objects
//

struct ValueHolder : public Object {
    TypePtr type;
    char *buf;
    ValueHolder(TypePtr type);
    ~ValueHolder();
};


bool objectEquals(ObjectPtr a, ObjectPtr b);
int objectHash(ObjectPtr a);

template <typename T>
bool objectVectorEquals(const vector<Pointer<T> > &a,
                        const vector<Pointer<T> > &b) {
    if (a.size() != b.size()) return false;
    for (unsigned i = 0; i < a.size(); ++i) {
        if (!objectEquals(a[i].ptr(), b[i].ptr()))
            return false;
    }
    return true;
}

template <typename T>
int objectVectorHash(const vector<Pointer<T> > &a) {
    int h = 0;
    for (unsigned i = 0; i < a.size(); ++i)
        h += objectHash(a[i].ptr());
    return h;
}

struct ObjectTableNode {
    vector<ObjectPtr> key;
    ObjectPtr value;
    ObjectTableNode(const vector<ObjectPtr> &key,
                    ObjectPtr value)
        : key(key), value(value) {}
};

struct ObjectTable : public Object {
    vector< vector<ObjectTableNode> > buckets;
    unsigned size;
public :
    ObjectTable() : Object(DONT_CARE), size(0) {}
    ObjectPtr &lookup(const vector<ObjectPtr> &key);
private :
    void rehash();
};



//
// types module
//

struct Type : public Object {
    int typeKind;
    llvm::Type *llType;
    bool defined;

    bool typeInfoInitialized;
    size_t typeSize;
    size_t typeAlignment;

    bool overloadsInitialized;
    vector<OverloadPtr> overloads;

    Type(int typeKind)
        : Object(TYPE), typeKind(typeKind),
          llType(NULL), defined(false),
          typeInfoInitialized(false), overloadsInitialized(false) {}
};

enum TypeKind {
    BOOL_TYPE,
    INTEGER_TYPE,
    FLOAT_TYPE,
    COMPLEX_TYPE,
    POINTER_TYPE,
    CODE_POINTER_TYPE,
    CCODE_POINTER_TYPE,
    ARRAY_TYPE,
    VEC_TYPE,
    TUPLE_TYPE,
    UNION_TYPE,
    RECORD_TYPE,
    VARIANT_TYPE,
    STATIC_TYPE,
    ENUM_TYPE
};

struct BoolType : public Type {
    BoolType() :
        Type(BOOL_TYPE) {}
};

struct IntegerType : public Type {
    int bits;
    bool isSigned;
    IntegerType(int bits, bool isSigned)
        : Type(INTEGER_TYPE), bits(bits), isSigned(isSigned) {}
};

struct FloatType : public Type {
    int bits;
    FloatType(int bits)
        : Type(FLOAT_TYPE), bits(bits) {}
};

struct ComplexType : public Type {
    int bits;
    const llvm::StructLayout *layout;
    ComplexType(int bits)
        : Type(COMPLEX_TYPE), bits(bits), layout(NULL) {}
};

struct PointerType : public Type {
    TypePtr pointeeType;
    PointerType(TypePtr pointeeType)
        : Type(POINTER_TYPE), pointeeType(pointeeType) {}
};

struct CodePointerType : public Type {
    vector<TypePtr> argTypes;

    vector<bool> returnIsRef;
    vector<TypePtr> returnTypes;

    CodePointerType(const vector<TypePtr> &argTypes,
                    const vector<bool> &returnIsRef,
                    const vector<TypePtr> &returnTypes)
        : Type(CODE_POINTER_TYPE), argTypes(argTypes),
          returnIsRef(returnIsRef), returnTypes(returnTypes) {}
};

struct CCodePointerType : public Type {
    CallingConv callingConv;
    vector<TypePtr> argTypes;
    bool hasVarArgs;
    TypePtr returnType; // NULL if void return
    CCodePointerType(CallingConv callingConv,
                     const vector<TypePtr> &argTypes,
                     bool hasVarArgs,
                     TypePtr returnType)
        : Type(CCODE_POINTER_TYPE), callingConv(callingConv),
          argTypes(argTypes), hasVarArgs(hasVarArgs),
          returnType(returnType) {}
};

struct ArrayType : public Type {
    TypePtr elementType;
    int size;
    ArrayType(TypePtr elementType, int size)
        : Type(ARRAY_TYPE), elementType(elementType), size(size) {}
};

struct VecType : public Type {
    TypePtr elementType;
    int size;
    VecType(TypePtr elementType, int size)
        : Type(VEC_TYPE), elementType(elementType), size(size) {}
};

struct TupleType : public Type {
    vector<TypePtr> elementTypes;
    const llvm::StructLayout *layout;
    TupleType()
        : Type(TUPLE_TYPE), layout(NULL) {}
    TupleType(const vector<TypePtr> &elementTypes)
        : Type(TUPLE_TYPE), elementTypes(elementTypes),
          layout(NULL) {}
};

struct UnionType : public Type {
    vector<TypePtr> memberTypes;
    UnionType()
        : Type(UNION_TYPE) {}
    UnionType(const vector<TypePtr> &memberTypes)
        : Type(UNION_TYPE), memberTypes(memberTypes)
        {}
};

struct RecordType : public Type {
    RecordPtr record;
    vector<ObjectPtr> params;

    bool fieldsInitialized;
    vector<IdentifierPtr> fieldNames;
    vector<TypePtr> fieldTypes;
    map<string, size_t> fieldIndexMap;

    const llvm::StructLayout *layout;

    RecordType(RecordPtr record)
        : Type(RECORD_TYPE), record(record), fieldsInitialized(false),
          layout(NULL) {}
    RecordType(RecordPtr record, const vector<ObjectPtr> &params)
        : Type(RECORD_TYPE), record(record), params(params),
          fieldsInitialized(false), layout(NULL) {}
};

struct VariantType : public Type {
    VariantPtr variant;
    vector<ObjectPtr> params;

    bool initialized;
    vector<TypePtr> memberTypes;
    TypePtr reprType;

    VariantType(VariantPtr variant)
        : Type(VARIANT_TYPE), variant(variant),
          initialized(false) {}
    VariantType(VariantPtr variant, const vector<ObjectPtr> &params)
        : Type(VARIANT_TYPE), variant(variant), params(params),
          initialized(false) {}
};

struct StaticType : public Type {
    ObjectPtr obj;
    StaticType(ObjectPtr obj)
        : Type(STATIC_TYPE), obj(obj) {}
};

struct EnumType : public Type {
    EnumerationPtr enumeration;
    bool initialized;

    EnumType(EnumerationPtr enumeration)
        : Type(ENUM_TYPE), enumeration(enumeration), initialized(false) {}
};


extern TypePtr boolType;
extern TypePtr int8Type;
extern TypePtr int16Type;
extern TypePtr int32Type;
extern TypePtr int64Type;
extern TypePtr int128Type;
extern TypePtr uint8Type;
extern TypePtr uint16Type;
extern TypePtr uint32Type;
extern TypePtr uint64Type;
extern TypePtr uint128Type;
extern TypePtr float32Type;
extern TypePtr float64Type;
extern TypePtr float80Type;
extern TypePtr float128Type;
extern TypePtr complex32Type;
extern TypePtr complex64Type;
extern TypePtr complex80Type;
extern TypePtr complex128Type;

// aliases
extern TypePtr cIntType;
extern TypePtr cSizeTType;
extern TypePtr cPtrDiffTType;

void initTypes();

TypePtr integerType(int bits, bool isSigned);
TypePtr intType(int bits);
TypePtr uintType(int bits);
TypePtr floatType(int bits);
TypePtr complexType(int bits);
TypePtr pointerType(TypePtr pointeeType);
TypePtr codePointerType(const vector<TypePtr> &argTypes,
                        const vector<bool> &returnIsRef,
                        const vector<TypePtr> &returnTypes);
TypePtr cCodePointerType(CallingConv callingConv,
                         const vector<TypePtr> &argTypes,
                         bool hasVarArgs,
                         TypePtr returnType);
TypePtr arrayType(TypePtr elememtType, int size);
TypePtr vecType(TypePtr elementType, int size);
TypePtr tupleType(const vector<TypePtr> &elementTypes);
TypePtr unionType(const vector<TypePtr> &memberTypes);
TypePtr recordType(RecordPtr record, const vector<ObjectPtr> &params);
TypePtr variantType(VariantPtr variant, const vector<ObjectPtr> &params);
TypePtr staticType(ObjectPtr obj);
TypePtr enumType(EnumerationPtr enumeration);

bool isPrimitiveType(TypePtr t);
bool isPrimitiveAggregateType(TypePtr t);
bool isPrimitiveAggregateTooLarge(TypePtr t);
bool isPointerOrCodePointerType(TypePtr t);
bool isStaticOrTupleOfStatics(TypePtr t);

void initializeRecordFields(RecordTypePtr t);
const vector<IdentifierPtr> &recordFieldNames(RecordTypePtr t);
const vector<TypePtr> &recordFieldTypes(RecordTypePtr t);
const map<string, size_t> &recordFieldIndexMap(RecordTypePtr t);

const vector<TypePtr> &variantMemberTypes(VariantTypePtr t);
TypePtr variantReprType(VariantTypePtr t);

void initializeEnumType(EnumTypePtr t);

const llvm::StructLayout *tupleTypeLayout(TupleType *t);
const llvm::StructLayout *complexTypeLayout(ComplexType *t);
const llvm::StructLayout *recordTypeLayout(RecordType *t);

llvm::Type *llvmIntType(int bits);
llvm::Type *llvmFloatType(int bits);
llvm::Type *llvmPointerType(llvm::Type *llType);
llvm::Type *llvmPointerType(TypePtr t);
llvm::Type *llvmArrayType(llvm::Type *llType, int size);
llvm::Type *llvmArrayType(TypePtr type, int size);
llvm::Type *llvmVoidType();

llvm::Type *llvmType(TypePtr t);

size_t typeSize(TypePtr t);
size_t typeAlignment(TypePtr t);
void typePrint(ostream &out, TypePtr t);
string typeName(TypePtr t);



//
// Pattern
//

enum PatternKind {
    PATTERN_CELL,
    PATTERN_STRUCT
};

struct Pattern : public Object {
    PatternKind kind;
    Pattern(PatternKind kind)
        : Object(PATTERN), kind(kind) {}
};

struct PatternCell : public Pattern {
    ObjectPtr obj;
    PatternCell(ObjectPtr obj)
        : Pattern(PATTERN_CELL), obj(obj) {}
};

struct PatternStruct : public Pattern {
    ObjectPtr head;
    MultiPatternPtr params;
    PatternStruct(ObjectPtr head, MultiPatternPtr params)
        : Pattern(PATTERN_STRUCT), head(head), params(params) {}
};


enum MultiPatternKind {
    MULTI_PATTERN_CELL,
    MULTI_PATTERN_LIST
};

struct MultiPattern : public Object {
    MultiPatternKind kind;
    MultiPattern(MultiPatternKind kind)
        : Object(MULTI_PATTERN), kind(kind) {}
};

struct MultiPatternCell : public MultiPattern {
    MultiPatternPtr data;
    MultiPatternCell(MultiPatternPtr data)
        : MultiPattern(MULTI_PATTERN_CELL), data(data) {}
};

struct MultiPatternList : public MultiPattern {
    vector<PatternPtr> items;
    MultiPatternPtr tail;
    MultiPatternList(const vector<PatternPtr> &items,
                     MultiPatternPtr tail)
        : MultiPattern(MULTI_PATTERN_LIST), items(items), tail(tail) {}
    MultiPatternList(MultiPatternPtr tail)
        : MultiPattern(MULTI_PATTERN_LIST), tail(tail) {}
    MultiPatternList(PatternPtr x)
        : MultiPattern(MULTI_PATTERN_LIST) {
        items.push_back(x);
    }
    MultiPatternList()
        : MultiPattern(MULTI_PATTERN_LIST) {}
};



//
// MultiStatic
//

struct MultiStatic : public Object {
    vector<ObjectPtr> values;
    MultiStatic()
        : Object(MULTI_STATIC) {}
    MultiStatic(ObjectPtr x)
        : Object(MULTI_STATIC) {
        values.push_back(x);
    }
    MultiStatic(const vector<ObjectPtr> &values)
        : Object(MULTI_STATIC), values(values) {}
    unsigned size() { return values.size(); }
    void add(ObjectPtr x) { values.push_back(x); }
    void add(MultiStaticPtr x) {
        values.insert(values.end(), x->values.begin(), x->values.end());
    }
};



//
// desugar
//

ExprPtr desugarCharLiteral(char c);
ExprPtr desugarFieldRef(FieldRefPtr x);
ExprPtr desugarStaticIndexing(StaticIndexingPtr x);
ExprPtr desugarUnaryOp(UnaryOpPtr x);
ExprPtr desugarBinaryOp(BinaryOpPtr x);
ExprPtr desugarIfExpr(IfExprPtr x);
ExprPtr desugarStaticExpr(StaticExprPtr x);
ExprPtr updateOperatorExpr(int op);
StatementPtr desugarForStatement(ForPtr x);
StatementPtr desugarCatchBlocks(const vector<CatchPtr> &catchBlocks);
StatementPtr desugarSwitchStatement(SwitchPtr x);

ExprListPtr desugarEvalExpr(EvalExprPtr eval, EnvPtr env);
vector<StatementPtr> const &desugarEvalStatement(EvalStatementPtr eval, EnvPtr env);
vector<TopLevelItemPtr> const &desugarEvalTopLevel(EvalTopLevelPtr eval, EnvPtr env);


//
// patterns
//

ObjectPtr derefDeep(PatternPtr x);
MultiStaticPtr derefDeep(MultiPatternPtr x);

bool unifyObjObj(ObjectPtr a, ObjectPtr b);
bool unifyObjPattern(ObjectPtr a, PatternPtr b);
bool unifyPatternObj(PatternPtr a, ObjectPtr b);
bool unify(PatternPtr a, PatternPtr b);
bool unifyMulti(MultiPatternPtr a, MultiStaticPtr b);
bool unifyMulti(MultiPatternPtr a, MultiPatternPtr b);
bool unifyMulti(MultiPatternListPtr a, unsigned indexA,
                MultiPatternPtr b);
bool unifyMulti(MultiPatternPtr a,
                MultiPatternListPtr b, unsigned indexB);
bool unifyMulti(MultiPatternListPtr a, unsigned indexA,
                MultiPatternListPtr b, unsigned indexB);
bool unifyEmpty(MultiPatternListPtr x, unsigned index);
bool unifyEmpty(MultiPatternPtr x);

PatternPtr evaluateOnePattern(ExprPtr expr, EnvPtr env);
PatternPtr evaluateAliasPattern(GlobalAliasPtr x, MultiPatternPtr params);
MultiPatternPtr evaluateMultiPattern(ExprListPtr exprs, EnvPtr env);

void patternPrint(ostream &out, PatternPtr x);
void patternPrint(ostream &out, MultiPatternPtr x);



//
// lambdas
//

void initializeLambda(LambdaPtr x, EnvPtr env);



//
// match invoke
//

enum MatchCode {
    MATCH_SUCCESS,
    MATCH_CALLABLE_ERROR,
    MATCH_ARITY_ERROR,
    MATCH_ARGUMENT_ERROR,
    MATCH_PREDICATE_ERROR
};

struct MatchResult : public Object {
    int matchCode;
    MatchResult(int matchCode)
        : Object(DONT_CARE), matchCode(matchCode) {}
};
typedef Pointer<MatchResult> MatchResultPtr;

struct MatchSuccess : public MatchResult {
    bool callByName;
    bool isInline;
    CodePtr code;
    EnvPtr env;

    ObjectPtr callable;
    vector<TypePtr> argsKey;

    vector<TypePtr> fixedArgTypes;
    vector<IdentifierPtr> fixedArgNames;
    IdentifierPtr varArgName;
    vector<TypePtr> varArgTypes;
    MatchSuccess(bool callByName, bool isInline, CodePtr code, EnvPtr env,
                 ObjectPtr callable, const vector<TypePtr> &argsKey)
        : MatchResult(MATCH_SUCCESS), callByName(callByName),
          isInline(isInline), code(code), env(env), callable(callable),
          argsKey(argsKey) {}
};
typedef Pointer<MatchSuccess> MatchSuccessPtr;

struct MatchCallableError : public MatchResult {
    MatchCallableError()
        : MatchResult(MATCH_CALLABLE_ERROR) {}
};

struct MatchArityError : public MatchResult {
    MatchArityError()
        : MatchResult(MATCH_ARITY_ERROR) {}
};

struct MatchArgumentError : public MatchResult {
    unsigned argIndex;
    FormalArgPtr argument;
    MatchArgumentError(unsigned argIndex, FormalArgPtr argument)
        : MatchResult(MATCH_ARGUMENT_ERROR), argIndex(argIndex), argument(argument) {}
};

struct MatchPredicateError : public MatchResult {
    MatchPredicateError()
        : MatchResult(MATCH_PREDICATE_ERROR) {}
};

MatchResultPtr matchInvoke(OverloadPtr overload,
                           ObjectPtr callable,
                           const vector<TypePtr> &argsKey);

void printMatchError(ostream &os, MatchResultPtr result);



//
// invoke tables
//

const vector<OverloadPtr> &callableOverloads(ObjectPtr x);

struct InvokeSet;

struct InvokeEntry : public Object {
    InvokeSet *parent;
    ObjectPtr callable;
    vector<TypePtr> argsKey;
    vector<bool> forwardedRValueFlags;

    bool analyzed;
    bool analyzing;

    CodePtr origCode;
    CodePtr code;
    EnvPtr env;
    EnvPtr interfaceEnv;
    vector<TypePtr> fixedArgTypes;
    vector<IdentifierPtr> fixedArgNames;
    IdentifierPtr varArgName;
    vector<TypePtr> varArgTypes;

    bool callByName; // if callByName the rest of InvokeEntry is not set
    bool isInline;

    ObjectPtr analysis;
    vector<bool> returnIsRef;
    vector<TypePtr> returnTypes;

    llvm::Function *llvmFunc;
    llvm::Function *llvmCWrapper;

    InvokeEntry(InvokeSet *parent,
                ObjectPtr callable,
                const vector<TypePtr> &argsKey)
        : Object(DONT_CARE),
          parent(parent),
          callable(callable), argsKey(argsKey),
          analyzed(false), analyzing(false), callByName(false),
          isInline(false), llvmFunc(NULL), llvmCWrapper(NULL) {}
};
typedef Pointer<InvokeEntry> InvokeEntryPtr;

extern vector<OverloadPtr> patternOverloads;

struct InvokeSet : public Object {
    ObjectPtr callable;
    vector<TypePtr> argsKey;
    OverloadPtr interface;
    vector<OverloadPtr> overloads;

    vector<MatchSuccessPtr> matches;
    unsigned nextOverloadIndex;

    map<vector<ValueTempness>, InvokeEntryPtr> tempnessMap;
    map<vector<ValueTempness>, InvokeEntryPtr> tempnessMap2;

    InvokeSet(ObjectPtr callable,
              const vector<TypePtr> &argsKey,
              OverloadPtr symbolInterface,
              const vector<OverloadPtr> &symbolOverloads)
        : Object(DONT_CARE), callable(callable), argsKey(argsKey),
          interface(symbolInterface),
          overloads(symbolOverloads), nextOverloadIndex(0)
    {
        overloads.insert(overloads.end(), patternOverloads.begin(), patternOverloads.end());
    }
};
typedef Pointer<InvokeSet> InvokeSetPtr;

typedef vector< pair<OverloadPtr, MatchResultPtr> > MatchFailureVector;

struct MatchFailureError {
    MatchFailureVector failures;
    bool failedInterface;

    MatchFailureError() : failedInterface(false) {}
};

InvokeSetPtr lookupInvokeSet(ObjectPtr callable,
                             const vector<TypePtr> &argsKey);
InvokeEntryPtr lookupInvokeEntry(ObjectPtr callable,
                                 const vector<TypePtr> &argsKey,
                                 const vector<ValueTempness> &argsTempness,
                                 MatchFailureError &failures);

void matchFailureError(MatchFailureError const &err);



//
// constructors
//

bool isOverloadablePrimOp(ObjectPtr x);
vector<OverloadPtr> &primOpOverloads(PrimOpPtr x);
void addPrimOpOverload(PrimOpPtr x, OverloadPtr overload);
void addPatternOverload(OverloadPtr x);
void initTypeOverloads(TypePtr t);
void initBuiltinConstructor(RecordPtr x);



//
// literals
//

ValueHolderPtr parseIntLiteral(ModulePtr module, IntLiteral *x);
ValueHolderPtr parseFloatLiteral(ModulePtr module, FloatLiteral *x);


//
// analyzer
//

struct PValue : public Object {
    TypePtr type;
    bool isTemp;
    PValue(TypePtr type, bool isTemp)
        : Object(PVALUE), type(type), isTemp(isTemp) {}
};

struct MultiPValue : public Object {
    vector<PValuePtr> values;
    MultiPValue()
        : Object(MULTI_PVALUE) {}
    MultiPValue(PValuePtr pv)
        : Object(MULTI_PVALUE) {
        values.push_back(pv);
    }
    MultiPValue(const vector<PValuePtr> &values)
        : Object(MULTI_PVALUE), values(values) {}
    unsigned size() { return values.size(); }
    void add(PValuePtr x) { values.push_back(x); }
    void add(MultiPValuePtr x) {
        values.insert(values.end(), x->values.begin(), x->values.end());
    }
};

void disableAnalysisCaching();
void enableAnalysisCaching();

struct AnalysisCachingDisabler {
    AnalysisCachingDisabler() { disableAnalysisCaching(); }
    ~AnalysisCachingDisabler() { enableAnalysisCaching(); }
};


PValuePtr safeAnalyzeOne(ExprPtr expr, EnvPtr env);
MultiPValuePtr safeAnalyzeMulti(ExprListPtr exprs, EnvPtr env, unsigned wantCount);
MultiPValuePtr safeAnalyzeExpr(ExprPtr expr, EnvPtr env);
MultiPValuePtr safeAnalyzeIndexingExpr(ExprPtr indexable,
                                       ExprListPtr args,
                                       EnvPtr env);
MultiPValuePtr safeAnalyzeMultiArgs(ExprListPtr exprs,
                                    EnvPtr env,
                                    vector<unsigned> &dispatchIndices);
InvokeEntryPtr safeAnalyzeCallable(ObjectPtr x,
                                   const vector<TypePtr> &argsKey,
                                   const vector<ValueTempness> &argsTempness);
MultiPValuePtr safeAnalyzeCallByName(InvokeEntryPtr entry,
                                     ExprPtr callable,
                                     ExprListPtr args,
                                     EnvPtr env);
MultiPValuePtr safeAnalyzeGVarInstance(GVarInstancePtr x);

MultiPValuePtr analyzeMulti(ExprListPtr exprs, EnvPtr env, unsigned wantCount);
PValuePtr analyzeOne(ExprPtr expr, EnvPtr env);

MultiPValuePtr analyzeMultiArgs(ExprListPtr exprs,
                                EnvPtr env,
                                vector<unsigned> &dispatchIndices);
PValuePtr analyzeOneArg(ExprPtr x,
                        EnvPtr env,
                        unsigned startIndex,
                        vector<unsigned> &dispatchIndices);
MultiPValuePtr analyzeArgExpr(ExprPtr x,
                              EnvPtr env,
                              unsigned startIndex,
                              vector<unsigned> &dispatchIndices);

MultiPValuePtr analyzeExpr(ExprPtr expr, EnvPtr env);
MultiPValuePtr analyzeStaticObject(ObjectPtr x);

GVarInstancePtr lookupGVarInstance(GlobalVariablePtr x,
                                   const vector<ObjectPtr> &params);
GVarInstancePtr defaultGVarInstance(GlobalVariablePtr x);
GVarInstancePtr analyzeGVarIndexing(GlobalVariablePtr x,
                                    ExprListPtr args,
                                    EnvPtr env);
MultiPValuePtr analyzeGVarInstance(GVarInstancePtr x);

PValuePtr analyzeExternalVariable(ExternalVariablePtr x);
void analyzeExternalProcedure(ExternalProcedurePtr x);
void verifyAttributes(ExternalProcedurePtr x);
void verifyAttributes(ExternalVariablePtr x);
void verifyAttributes(ModulePtr x);
MultiPValuePtr analyzeIndexingExpr(ExprPtr indexable,
                                   ExprListPtr args,
                                   EnvPtr env);
bool unwrapByRef(TypePtr &t);
TypePtr constructType(ObjectPtr constructor, MultiStaticPtr args);
PValuePtr analyzeTypeConstructor(ObjectPtr obj, MultiStaticPtr args);
MultiPValuePtr analyzeAliasIndexing(GlobalAliasPtr x,
                                    ExprListPtr args,
                                    EnvPtr env);
void computeArgsKey(MultiPValuePtr args,
                    vector<TypePtr> &argsKey,
                    vector<ValueTempness> &argsTempness);
MultiPValuePtr analyzeReturn(const vector<bool> &returnIsRef,
                             const vector<TypePtr> &returnTypes);
MultiPValuePtr analyzeCallExpr(ExprPtr callable,
                               ExprListPtr args,
                               EnvPtr env);
MultiPValuePtr analyzeDispatch(ObjectPtr obj,
                               MultiPValuePtr args,
                               const vector<unsigned> &dispatchIndices);
MultiPValuePtr analyzeCallValue(PValuePtr callable,
                                MultiPValuePtr args);
MultiPValuePtr analyzeCallPointer(PValuePtr x,
                                  MultiPValuePtr args);
bool analyzeIsDefined(ObjectPtr x,
                      const vector<TypePtr> &argsKey,
                      const vector<ValueTempness> &argsTempness);
InvokeEntryPtr analyzeCallable(ObjectPtr x,
                               const vector<TypePtr> &argsKey,
                               const vector<ValueTempness> &argsTempness);

MultiPValuePtr analyzeCallByName(InvokeEntryPtr entry,
                                 ExprPtr callable,
                                 ExprListPtr args,
                                 EnvPtr env);

void analyzeCodeBody(InvokeEntryPtr entry);

struct AnalysisContext : public Object {
    bool hasRecursivePropagation;
    bool returnInitialized;
    vector<bool> returnIsRef;
    vector<TypePtr> returnTypes;
    AnalysisContext()
        : Object(DONT_CARE),
          hasRecursivePropagation(false),
          returnInitialized(false) {}
};
typedef Pointer<AnalysisContext> AnalysisContextPtr;

enum StatementAnalysis {
    SA_FALLTHROUGH,
    SA_RECURSIVE,
    SA_TERMINATED
};

StatementAnalysis analyzeStatement(StatementPtr stmt, EnvPtr env, AnalysisContextPtr ctx);
void initializeStaticForClones(StaticForPtr x, unsigned count);
EnvPtr analyzeBinding(BindingPtr x, EnvPtr env);
bool returnKindToByRef(ReturnKind returnKind, PValuePtr pv);

MultiPValuePtr analyzePrimOp(PrimOpPtr x, MultiPValuePtr args);

ObjectPtr unwrapStaticType(TypePtr t);


//
// evaluator
//

bool staticToType(ObjectPtr x, TypePtr &out);
TypePtr staticToType(MultiStaticPtr x, unsigned index);

void evaluateReturnSpecs(const vector<ReturnSpecPtr> &returnSpecs,
                         ReturnSpecPtr varReturnSpec,
                         EnvPtr env,
                         vector<bool> &isRef,
                         vector<TypePtr> &types);

MultiStaticPtr evaluateExprStatic(ExprPtr expr, EnvPtr env);
ObjectPtr evaluateOneStatic(ExprPtr expr, EnvPtr env);
MultiStaticPtr evaluateMultiStatic(ExprListPtr exprs, EnvPtr env);

TypePtr evaluateType(ExprPtr expr, EnvPtr env);
void evaluateMultiType(ExprListPtr exprs, EnvPtr env, vector<TypePtr> &out);
IdentifierPtr evaluateIdentifier(ExprPtr expr, EnvPtr env);
bool evaluateBool(ExprPtr expr, EnvPtr env);
void evaluateToplevelPredicate(const vector<PatternVar> &patternVars,
    ExprPtr expr, EnvPtr env);

ValueHolderPtr intToValueHolder(int x);
ValueHolderPtr sizeTToValueHolder(size_t x);
ValueHolderPtr ptrDiffTToValueHolder(ptrdiff_t x);
ValueHolderPtr boolToValueHolder(bool x);

ObjectPtr makeTupleValue(const vector<ObjectPtr> &elements);
void extractTupleElements(EValuePtr ev,
                          vector<ObjectPtr> &elements);
ObjectPtr evalueToStatic(EValuePtr ev);

struct EValue : public Object {
    TypePtr type;
    char *addr;
    bool forwardedRValue;
    EValue(TypePtr type, char *addr)
        : Object(EVALUE), type(type), addr(addr),
          forwardedRValue(false) {}
};

struct MultiEValue : public Object {
    vector<EValuePtr> values;
    MultiEValue()
        : Object(MULTI_EVALUE) {}
    MultiEValue(EValuePtr pv)
        : Object(MULTI_EVALUE) {
        values.push_back(pv);
    }
    MultiEValue(const vector<EValuePtr> &values)
        : Object(MULTI_EVALUE), values(values) {}
    unsigned size() { return values.size(); }
    void add(EValuePtr x) { values.push_back(x); }
    void add(MultiEValuePtr x) {
        values.insert(values.end(), x->values.begin(), x->values.end());
    }
};

void evalValueInit(EValuePtr dest);
void evalValueDestroy(EValuePtr dest);
void evalValueCopy(EValuePtr dest, EValuePtr src);
void evalValueMove(EValuePtr dest, EValuePtr src);
void evalValueAssign(EValuePtr dest, EValuePtr src);
void evalValueMoveAssign(EValuePtr dest, EValuePtr src);
bool evalToBoolFlag(EValuePtr a);

int evalMarkStack();
void evalDestroyStack(int marker);
void evalPopStack(int marker);
void evalDestroyAndPopStack(int marker);
EValuePtr evalAllocValue(TypePtr t);



//
// codegen
//

extern llvm::Module *llvmModule;
extern llvm::ExecutionEngine *llvmEngine;
extern const llvm::TargetData *llvmTargetData;

bool initLLVM(std::string const &targetTriple);

bool inlineEnabled();
void setInlineEnabled(bool enabled);
bool exceptionsEnabled();
void setExceptionsEnabled(bool enabled);

struct CValue : public Object {
    TypePtr type;
    llvm::Value *llValue;
    bool forwardedRValue;
    CValue(TypePtr type, llvm::Value *llValue)
        : Object(CVALUE), type(type), llValue(llValue),
          forwardedRValue(false)
    {
        llvmType(type); // force full definition of type
    }
};

struct MultiCValue : public Object {
    vector<CValuePtr> values;
    MultiCValue()
        : Object(MULTI_CVALUE) {}
    MultiCValue(CValuePtr pv)
        : Object(MULTI_CVALUE) {
        values.push_back(pv);
    }
    MultiCValue(const vector<CValuePtr> &values)
        : Object(MULTI_CVALUE), values(values) {}
    unsigned size() { return values.size(); }
    void add(CValuePtr x) { values.push_back(x); }
    void add(MultiCValuePtr x) {
        values.insert(values.end(), x->values.begin(), x->values.end());
    }
};

struct JumpTarget {
    llvm::BasicBlock *block;
    int stackMarker;
    int useCount;
    JumpTarget() : block(NULL), stackMarker(-1), useCount(0) {}
    JumpTarget(llvm::BasicBlock *block, int stackMarker)
        : block(block), stackMarker(stackMarker), useCount(0) {}
};

struct CReturn {
    bool byRef;
    TypePtr type;
    CValuePtr value;
    CReturn(bool byRef, TypePtr type, CValuePtr value)
        : byRef(byRef), type(type), value(value) {}
};

struct StackSlot {
    llvm::Type *llType;
    llvm::Value *llValue;
    StackSlot(llvm::Type *llType, llvm::Value *llValue)
        : llType(llType), llValue(llValue) {}
};

enum ValueStackEntryType {
    LOCAL_VALUE,
    FINALLY_STATEMENT,
    ONERROR_STATEMENT,
};

struct ValueStackEntry {
    ValueStackEntryType type;
    CValuePtr value;
    EnvPtr statementEnv;
    StatementPtr statement;

    explicit ValueStackEntry(CValuePtr value)
        : type(LOCAL_VALUE), value(value), statementEnv(NULL), statement(NULL) {}
    ValueStackEntry(ValueStackEntryType type,
        EnvPtr statementEnv,
        StatementPtr statement)
        : type(type), value(NULL), statementEnv(statementEnv), statement(statement)
    {
        assert(type != LOCAL_VALUE);
    }
};

struct CodegenContext : public Object {
    llvm::Function *llvmFunc;
    llvm::IRBuilder<> *initBuilder;
    llvm::IRBuilder<> *builder;

    vector<ValueStackEntry> valueStack;
    llvm::Value *valueForStatics;

    vector<StackSlot> allocatedSlots;
    vector<StackSlot> discardedSlots;

    vector< vector<CReturn> > returnLists;
    vector<JumpTarget> returnTargets;
    map<string, JumpTarget> labels;
    vector<JumpTarget> breaks;
    vector<JumpTarget> continues;
    vector<JumpTarget> exceptionTargets;
    bool checkExceptions;

    CodegenContext(llvm::Function *llvmFunc)
        : Object(DONT_CARE),
          llvmFunc(llvmFunc),
          initBuilder(NULL),
          builder(NULL),
          valueForStatics(NULL),
          checkExceptions(true) {}

    ~CodegenContext() {
        delete builder;
        delete initBuilder;
    }
};

typedef Pointer<CodegenContext> CodegenContextPtr;

void codegenGVarInstance(GVarInstancePtr x);
void codegenExternalVariable(ExternalVariablePtr x);
void codegenExternalProcedure(ExternalProcedurePtr x, bool codegenBody);

InvokeEntryPtr codegenCallable(ObjectPtr x,
                               const vector<TypePtr> &argsKey,
                               const vector<ValueTempness> &argsTempness);
void codegenCodeBody(InvokeEntryPtr entry);
void codegenCWrapper(InvokeEntryPtr entry);

void codegenEntryPoints(ModulePtr module, bool importedExternals);
void codegenMain(ModulePtr module);

static ExprPtr implicitUnpackExpr(unsigned wantCount, ExprListPtr exprs) {
    if (wantCount >= 1 && exprs->size() == 1 && exprs->exprs[0]->exprKind != UNPACK)
        return exprs->exprs[0];
    else
        return NULL;
}

// externals

struct ExternalTarget : public Object {
    ExternalTarget() : Object(DONT_CARE) {}
    virtual ~ExternalTarget() {}

    virtual llvm::CallingConv::ID callingConvention(CallingConv conv) = 0;

    // for generating C function declarations
    virtual llvm::Type *pushReturnType(TypePtr type,
                                       vector<llvm::Type *> &llArgTypes,
                                       vector< pair<unsigned, llvm::Attributes> > &llAttributes) = 0;
    virtual void pushArgumentType(TypePtr type,
                                  vector<llvm::Type *> &llArgTypes,
                                  vector< pair<unsigned, llvm::Attributes> > &llAttributes) = 0;

    // for generating C function definitions
    virtual void allocReturnValue(TypePtr type,
                                  llvm::Function::arg_iterator &ai,
                                  vector<CReturn> &returns,
                                  CodegenContextPtr ctx) = 0;
    virtual CValuePtr allocArgumentValue(TypePtr type,
                                         string const &name,
                                         llvm::Function::arg_iterator &ai,
                                         CodegenContextPtr ctx) = 0;
    virtual void returnStatement(TypePtr type,
                                 vector<CReturn> &returns,
                                 CodegenContextPtr ctx) = 0;

    // for calling C functions
    virtual void loadStructRetArgument(TypePtr type,
                                       vector<llvm::Value *> &llArgs,
                                       vector< pair<unsigned, llvm::Attributes> > &llAttributes,
                                       CodegenContextPtr ctx,
                                       MultiCValuePtr out) = 0;
    virtual void loadArgument(CValuePtr cv,
                              vector<llvm::Value *> &llArgs,
                              vector< pair<unsigned, llvm::Attributes> > &llAttributes,
                              CodegenContextPtr ctx) = 0;
    virtual void loadVarArgument(CValuePtr cv,
                                 vector<llvm::Value *> &llArgs,
                                 vector< pair<unsigned, llvm::Attributes> > &llAttributes,
                                 CodegenContextPtr ctx) = 0;
    virtual void storeReturnValue(llvm::Value *callReturnValue,
                                  TypePtr returnType,
                                  CodegenContextPtr ctx,
                                  MultiCValuePtr out) = 0;
};

typedef Pointer<ExternalTarget> ExternalTargetPtr;

void initExternalTarget(string target);
ExternalTargetPtr getExternalTarget();

#endif
