/*
 * CS3201 Simple Static Analyzer
 * Copyright (C) 2011 Soares Chen Ruo Fei
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

#include <string>
#include <algorithm>

#include "impl/solvers/follows.h"
#include "impl/solvers/ifollows.h"
#include "impl/solvers/parent.h"
#include "impl/solvers/iparent.h"
#include "impl/solvers/call.h"
#include "impl/solvers/icall.h"
#include "impl/solvers/modifies.h"
#include "impl/solvers/uses.h"
#include "impl/solvers/next.h"
#include "impl/solvers/inext.h"
#include "impl/solvers/same_name.h"

#include "simple/util/solver_generator.h"
#include "simple/util/query_utils.h"
#include "simple/util/condition_utils.h"

#include "impl/parser/parser.h"
#include "impl/parser/pql_parser.h"
#include "impl/parser/iterator_tokenizer.h"

#include "impl/predicate.h"
#include "impl/linker.h"
#include "impl/processor.h"

namespace simple {
namespace impl {

using namespace simple;
using namespace simple::util;
using namespace simple::parser;

class SimplePqlFrontEnd {
  public:
    template <typename Iterator>
    SimplePqlFrontEnd(Iterator begin, Iterator end)
    {
        parse_source(begin, end);
        populate_solvers();
        populate_predicates();
    }

    template <typename Iterator, typename OutputIterator>
    void process_query(Iterator begin, Iterator end, OutputIterator out)
    {
        try {
            std::shared_ptr<QueryLinker> linker(new SimpleQueryLinker());

            SimplePqlParser parser(std::shared_ptr<SimpleTokenizer>(
                    new IteratorTokenizer<Iterator>(begin, end)),
                    _ast, _line_table, _solver_table, _pred_table);

            PqlQuerySet query = parser.parse_query();

            QueryProcessor processor(linker, query.predicates, _wildcard_pred);

            for(ClauseSet::iterator it = query.clauses.begin();
                it != query.clauses.end(); ++it)
            {
                processor.solve_clause(it->get());
            }

            format_selected(linker.get(), query, out);
        } catch(std::exception& e) {
            print("Internal error: ", out);
            print(e.what(), out);
        }
    }
  
  protected:
    template <typename OutputIterator>
    void print(const std::string& message, OutputIterator& out) {
        std::copy(message.begin(), message.end(), out);
    }

    template <typename SourceIterator>
    void parse_source(SourceIterator begin, SourceIterator end) 
    {
        SimpleParser parser(
            new IteratorTokenizer<SourceIterator>(begin, end));

        _ast = parser.parse_program();
        _line_table = parser.get_line_table();
    }

    void populate_solvers() {
        _solver_table["follows"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<FollowSolver>(new FollowSolver(_ast)));

        _solver_table["ifollows"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<IFollowSolver>(new IFollowSolver(_ast)));

        _solver_table["parent"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<ParentSolver>(new ParentSolver(_ast)));

        _solver_table["iparent"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<IParentSolver>(new IParentSolver(_ast)));

        _solver_table["call"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<CallSolver>(new CallSolver(_ast)));

        _solver_table["icall"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<ICallSolver>(new ICallSolver(_ast)));

        _solver_table["modifies"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<ModifiesSolver>(new ModifiesSolver(_ast)));

        _solver_table["uses"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<UsesSolver>(new UsesSolver(_ast)));
        
        std::shared_ptr<NextSolver> next_solver(new NextSolver(_ast));

        std::shared_ptr<INextSolver> inext_solver(new INextSolver(_ast, next_solver));

        _solver_table["next"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<NextSolver>(next_solver));
        _solver_table["inext"] = std::shared_ptr<QuerySolver>(
            new SimpleSolverGenerator<INextSolver>(inext_solver));
    }

    void populate_predicates() {
        _wildcard_pred = PredicatePtr(new SimpleWildCardPredicate(_ast));

        _pred_table["wildcard"] = _wildcard_pred;
        _pred_table["procedure"] = PredicatePtr(new SimpleProcPredicate(_ast));
        _pred_table["statement"] = PredicatePtr(new SimpleStatementPredicate(_ast));
        _pred_table["assign"] = PredicatePtr(new SimpleAssignmentPredicate(_ast));
        _pred_table["while"] = PredicatePtr(new SimpleWhilePredicate(_ast));
        _pred_table["if"] = PredicatePtr(new SimpleConditionalPredicate(_ast));
        _pred_table["call"] = PredicatePtr(new SimpleCallPredicate(_ast));
        _pred_table["var"] = PredicatePtr(new SimpleVariablePredicate(_ast));
        _pred_table["const"] = PredicatePtr(new SimpleConstantPredicate(_ast));
    }

    template <typename OutputIterator>
    void format_selected(QueryLinker *linker, PqlQuerySet& query, 
                OutputIterator& out) 
    {
        PqlSelector *selector = query.selector.get();

        if(is_selector<PqlBooleanSelector>(selector)) {
            if(linker->is_valid_state()) {
                print("true", out);
            } else {
                print("false", out);
            }
        } else if(is_selector<PqlSingleVarSelector>(selector)) {
            PqlSingleVarSelector *var_selector = selector_cast<
                            PqlSingleVarSelector>(selector);
            
            std::string qvar = var_selector->get_qvar_name();
            SimplePredicate *pred;

            if(query.predicates.count(qvar) > 0) {
                pred = query.predicates[qvar].get();
            } else {
                pred = _wildcard_pred.get();
            }

            ConditionSet conditions = linker->get_conditions(qvar, pred);
            print(qvar, out);
            print(" = ", out);

            for(ConditionSet::iterator it = conditions.begin(); 
                    it != conditions.end();)
            {
                print(condition_to_string(*it), out);
                if(++it != conditions.end()) {
                    print(", ", out);
                }
            }
        } else {
            print("Not implemented", out);
        }
    }

  private:
    SimpleRoot      _ast;
    SolverTable     _solver_table;
    PredicateTable  _pred_table;
    LineTable       _line_table;
    PredicatePtr    _wildcard_pred;
};

}
}
