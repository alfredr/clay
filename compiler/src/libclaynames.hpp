#ifndef __CLAYNAMES_HPP
#define __CLAYNAMES_HPP

#include "clay.hpp"


ObjectPtr primitive_addressOf();
ObjectPtr primitive_boolNot();
ObjectPtr primitive_Pointer();
ObjectPtr primitive_CodePointer();
ObjectPtr primitive_CCodePointer();
ObjectPtr primitive_VarArgsCCodePointer();
ObjectPtr primitive_StdCallCodePointer();
ObjectPtr primitive_FastCallCodePointer();
ObjectPtr primitive_Array();
ObjectPtr primitive_Vec();
ObjectPtr primitive_Tuple();
ObjectPtr primitive_Union();
ObjectPtr primitive_Static();

ExprPtr primitive_expr_addressOf();
ExprPtr primitive_expr_boolNot();
ExprPtr primitive_expr_Pointer();
ExprPtr primitive_expr_CodePointer();
ExprPtr primitive_expr_CCodePointer();
ExprPtr primitive_expr_VarArgsCCodePointer();
ExprPtr primitive_expr_StdCallCodePointer();
ExprPtr primitive_expr_FastCallCodePointer();
ExprPtr primitive_expr_Array();
ExprPtr primitive_expr_Vec();
ExprPtr primitive_expr_Tuple();
ExprPtr primitive_expr_Union();
ExprPtr primitive_expr_Static();

ObjectPtr prelude_dereference();
ObjectPtr prelude_plus();
ObjectPtr prelude_minus();
ObjectPtr prelude_add();
ObjectPtr prelude_subtract();
ObjectPtr prelude_multiply();
ObjectPtr prelude_divide();
ObjectPtr prelude_remainder();
ObjectPtr prelude_equalsP();
ObjectPtr prelude_notEqualsP();
ObjectPtr prelude_lesserP();
ObjectPtr prelude_lesserEqualsP();
ObjectPtr prelude_greaterP();
ObjectPtr prelude_greaterEqualsP();
ObjectPtr prelude_tupleLiteral();
ObjectPtr prelude_arrayLiteral();
ObjectPtr prelude_staticIndex();
ObjectPtr prelude_index();
ObjectPtr prelude_fieldRef();
ObjectPtr prelude_call();
ObjectPtr prelude_destroy();
ObjectPtr prelude_move();
ObjectPtr prelude_assign();
ObjectPtr prelude_addAssign();
ObjectPtr prelude_subtractAssign();
ObjectPtr prelude_multiplyAssign();
ObjectPtr prelude_divideAssign();
ObjectPtr prelude_remainderAssign();
ObjectPtr prelude_ByRef();
ObjectPtr prelude_setArgcArgv();
ObjectPtr prelude_callMain();
ObjectPtr prelude_Char();
ObjectPtr prelude_allocateShared();
ObjectPtr prelude_wrapStatic();
ObjectPtr prelude_iterator();
ObjectPtr prelude_hasNextP();
ObjectPtr prelude_next();
ObjectPtr prelude_throwValue();
ObjectPtr prelude_exceptionIsP();
ObjectPtr prelude_exceptionAs();
ObjectPtr prelude_exceptionAsAny();
ObjectPtr prelude_continueException();
ObjectPtr prelude_unhandledExceptionInExternal();
ObjectPtr prelude_exceptionInInitializer();
ObjectPtr prelude_exceptionInFinalizer();
ObjectPtr prelude_packMultiValuedFreeVarByRef();
ObjectPtr prelude_packMultiValuedFreeVar();
ObjectPtr prelude_unpackMultiValuedFreeVarAndDereference();
ObjectPtr prelude_unpackMultiValuedFreeVar();
ObjectPtr prelude_VariantRepr();
ObjectPtr prelude_variantTag();
ObjectPtr prelude_unsafeVariantIndex();
ObjectPtr prelude_invalidVariant();
ObjectPtr prelude_StringConstant();
ObjectPtr prelude_ifExpression();
ObjectPtr prelude_RecordWithPredicate();
ObjectPtr prelude_typeToRValue();
ObjectPtr prelude_typesToRValues();

ExprPtr prelude_expr_dereference();
ExprPtr prelude_expr_plus();
ExprPtr prelude_expr_minus();
ExprPtr prelude_expr_add();
ExprPtr prelude_expr_subtract();
ExprPtr prelude_expr_multiply();
ExprPtr prelude_expr_divide();
ExprPtr prelude_expr_remainder();
ExprPtr prelude_expr_equalsP();
ExprPtr prelude_expr_notEqualsP();
ExprPtr prelude_expr_lesserP();
ExprPtr prelude_expr_lesserEqualsP();
ExprPtr prelude_expr_greaterP();
ExprPtr prelude_expr_greaterEqualsP();
ExprPtr prelude_expr_tupleLiteral();
ExprPtr prelude_expr_arrayLiteral();
ExprPtr prelude_expr_staticIndex();
ExprPtr prelude_expr_index();
ExprPtr prelude_expr_fieldRef();
ExprPtr prelude_expr_call();
ExprPtr prelude_expr_destroy();
ExprPtr prelude_expr_move();
ExprPtr prelude_expr_assign();
ExprPtr prelude_expr_addAssign();
ExprPtr prelude_expr_subtractAssign();
ExprPtr prelude_expr_multiplyAssign();
ExprPtr prelude_expr_divideAssign();
ExprPtr prelude_expr_remainderAssign();
ExprPtr prelude_expr_ByRef();
ExprPtr prelude_expr_setArgcArgv();
ExprPtr prelude_expr_callMain();
ExprPtr prelude_expr_Char();
ExprPtr prelude_expr_allocateShared();
ExprPtr prelude_expr_wrapStatic();
ExprPtr prelude_expr_iterator();
ExprPtr prelude_expr_hasNextP();
ExprPtr prelude_expr_next();
ExprPtr prelude_expr_throwValue();
ExprPtr prelude_expr_exceptionIsP();
ExprPtr prelude_expr_exceptionAs();
ExprPtr prelude_expr_exceptionAsAny();
ExprPtr prelude_expr_continueException();
ExprPtr prelude_expr_unhandledExceptionInExternal();
ExprPtr prelude_expr_exceptionInInitializer();
ExprPtr prelude_expr_exceptionInFinalizer();
ExprPtr prelude_expr_packMultiValuedFreeVarByRef();
ExprPtr prelude_expr_packMultiValuedFreeVar();
ExprPtr prelude_expr_unpackMultiValuedFreeVarAndDereference();
ExprPtr prelude_expr_unpackMultiValuedFreeVar();
ExprPtr prelude_expr_VariantRepr();
ExprPtr prelude_expr_variantTag();
ExprPtr prelude_expr_unsafeVariantIndex();
ExprPtr prelude_expr_invalidVariant();
ExprPtr prelude_expr_StringConstant();
ExprPtr prelude_expr_ifExpression();
ExprPtr prelude_expr_RecordWithPredicate();
ExprPtr prelude_expr_typeToRValue();
ExprPtr prelude_expr_typesToRValues();


#endif
