/*
 * CS3201 Simple Static Analyzer
 * Copyright (C) 2011 Soares Chen Ruo Fei
 *  Contributor(s):
 *    Soares Chen Ruo Fei (original author)
 *    Daniel Le <GreenRecycleBin@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <set>
#include "simple/ast.h"
#include "simple/condition.h"
#include "simple/condition_set.h"

namespace simple {
namespace impl {

using namespace simple;

template <typename Solver>
class PartialValidateVisitorTraits {
  public:
    typedef bool                ResultType;
    typedef StatementAst        ContextType;

    template <typename Ast>
    static bool visit(Solver *solver, Ast *ast, 
                StatementAst *statement) 
    {
        return solver->template validate<Ast, StatementAst>(ast, statement);
    }
  private:
    PartialValidateVisitorTraits();
};

template <typename Solver>
class IndexVariableVisitorTraits {
  public:
    typedef std::set<SimpleVariable> ResultType;
    typedef int                    ContextType;

    template <typename Ast>
    static std::set<SimpleVariable> visit(
            Solver *solver, Ast *ast, int *context = NULL) 
    {
        return solver->template index_variables<Ast>(ast);
    }
  private:
    IndexVariableVisitorTraits();
};

template <typename Solver>
class SolveVariableVisitorTraits {
  public:
    typedef ConditionSet ResultType;
    typedef int            ContextType;

    template <typename Ast>
    static ConditionSet visit(Solver *solver, Ast *ast, int *context = NULL) {
        return solver->template solve_variable<Ast>(ast);
    }
  private:
    SolveVariableVisitorTraits();
};

template <typename Solver>
class SolveRightVisitorTraits {
  public:
    typedef ConditionSet ResultType;
    typedef int        ContextType;

    template <typename Ast>
    static ConditionSet visit(Solver *solver, Ast *ast, int *context = NULL) {
        return solver->template solve_right<Ast>(ast);
    }
  private:
    SolveRightVisitorTraits();
};

template <typename Solver>
class SolveLeftVisitorTraits {
  public:
    typedef ConditionSet ResultType;
    typedef int        ContextType;

    template <typename Ast>
    static ConditionSet visit(Solver *solver, Ast *ast, int *context = NULL) {
        return solver->template solve_left<Ast>(ast);
    }

  private:
    SolveLeftVisitorTraits();
};

template <typename Visitor, typename VisitorTraits>
class StatementVisitorGenerator : public StatementVisitor {
  public:
    typedef typename VisitorTraits::ResultType  ResultType;
    typedef typename VisitorTraits::ContextType ContextType;

    StatementVisitorGenerator(Visitor *visitor, ContextType *context = NULL) : 
        _visitor(visitor), _context(context) 
    { }

    virtual void visit_conditional(ConditionalAst *ast) {
        _result = VisitorTraits::template visit<ConditionalAst>(_visitor, ast, _context);
    }

    virtual void visit_while(WhileAst *ast) {
        _result = VisitorTraits::template visit<WhileAst>(_visitor, ast, _context);
    }

    virtual void visit_assignment(AssignmentAst *ast) {
        _result = VisitorTraits::template visit<AssignmentAst>(_visitor, ast, _context);
    }

    virtual void visit_call(CallAst *ast) {
        _result = VisitorTraits::template visit<CallAst>(_visitor, ast, _context);
    }

    typename VisitorTraits::ResultType return_result() {
        return std::move(_result);
    }

  private:
    Visitor *_visitor;
    ResultType _result;
    ContextType *_context;
};

template <typename Visitor, typename VisitorTraits>
class ExprVisitorGenerator : public ExprVisitor {
  public:
    typedef typename VisitorTraits::ResultType  ResultType;
    typedef typename VisitorTraits::ContextType ContextType;

    ExprVisitorGenerator(Visitor *visitor, ContextType *context = NULL) : 
        _visitor(visitor), _context(context) 
    { }

    virtual void visit_const(ConstAst *ast) {
        _result = VisitorTraits::template visit<ConstAst>(_visitor, ast, _context);
    }

    virtual void visit_variable(VariableAst *ast) {
        _result = VisitorTraits::template visit<VariableAst>(_visitor, ast, _context);
    }

    virtual void visit_binary_op(BinaryOpAst *ast) {
        _result = VisitorTraits::template visit<BinaryOpAst>(_visitor, ast, _context);
    }

    typename VisitorTraits::ResultType return_result() {
        return std::move(_result);
    }

  private:
    Visitor *_visitor;
    ResultType _result;
    ContextType *_context;
};

}
}
