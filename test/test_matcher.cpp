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

#include "gtest/gtest.h"
#include "impl/qvar.h"
#include "impl/matcher.h"
#include "impl/ast.h"
#include "impl/condition.h"
#include "impl/predicate.h"
#include "impl/solvers/modifies.h"
#include "impl/solvers/next.h"
#include "simple/util/solver_generator.h"
#include "simple/util/ast_utils.h"

namespace simple {
namespace test {

using namespace simple;
using namespace simple::impl;
using namespace simple::util;

TEST(MatcherTest, ModifiesTest) {
    /*
     * proc test {
     *   x = 1;  // stat1
     *   x = y;  // stat2
     * }
     */
    SimpleProcAst *proc = new SimpleProcAst("test");
    SimpleVariable var_x("x");
    SimpleVariable var_y("y");

    SimpleAssignmentAst *stat1 = new SimpleAssignmentAst();
    stat1->set_variable(var_x);
    stat1->set_expr(new SimpleConstAst(1));
    set_proc(stat1, proc);

    SimpleAssignmentAst *stat2 = new SimpleAssignmentAst();
    stat2->set_variable(var_x);
    stat2->set_expr(new SimpleVariableAst(var_y));
    set_next(stat1, stat2);
    
    SimpleRoot root(proc);
    
    SimpleSolverGenerator<ModifiesSolver> *solver = 
        new SimpleSolverGenerator<ModifiesSolver>(
            new ModifiesSolver(root));

    ModifiesSolver *modifies = solver->get_solver();

    ConditionSet expected_stats;
    expected_stats.insert(new SimpleStatementCondition(stat1));
    expected_stats.insert(new SimpleStatementCondition(stat2));
    expected_stats.insert(new SimpleProcCondition(proc));

    ConditionSet expected_vars(new SimpleVariableCondition(var_x));
    
    EXPECT_EQ(modifies->solve_left<SimpleVariable>(&var_x), expected_stats);
    EXPECT_EQ(modifies->solve_right<StatementAst>(stat1), expected_vars);
    EXPECT_EQ(modifies->solve_right<StatementAst>(stat2), expected_vars);

    /*
     * Modifies(stat1, V)
     * Modifies(V, S)
     *
     * Select all conditions that modifies the same variable that is
     * modified by stat1.
     */
    std::shared_ptr<SimplePredicate> pred(new SimpleWildCardPredicate(root));

    SimpleQueryMatcher matcher(solver);
    SimpleQueryVariable qvar_v("V", pred);
    SimpleQueryVariable qvar_s("S", pred);
    SimpleStatementCondition stat1_condition(stat1);

    matcher.solve_right(&stat1_condition, &qvar_v);
    EXPECT_EQ(qvar_v.get_conditions(), expected_vars);

    matcher.solve_both(&qvar_s, &qvar_v);
    EXPECT_EQ(qvar_v.get_conditions(), expected_vars);
    EXPECT_EQ(qvar_s.get_conditions(), expected_stats);
}

TEST(MatcherTest, NextTest) {
    /*
     * proc test {
     *   if i {     // condition
     *     x = 1    // stat1
     *     y = 2    // stat2
     *   } else {
     *     z = 3    // stat3
     *   }
     *   a = 4      // stat4
     * }
     */
    SimpleProcAst *proc = new SimpleProcAst("test");
    SimpleVariable var_x("x");
    SimpleVariable var_y("y");
    SimpleVariable var_z("z");
    SimpleVariable var_a("a");

    SimpleConditionalAst *condition = new SimpleConditionalAst();
    condition->set_line(1);
    set_proc(condition, proc);

    SimpleAssignmentAst *stat1 = new SimpleAssignmentAst();
    stat1->set_variable(var_x);
    stat1->set_expr(new SimpleConstAst(1));
    stat1->set_line(2);
    set_then_branch(stat1, condition);

    SimpleAssignmentAst *stat2 = new SimpleAssignmentAst();
    stat2->set_variable(var_y);
    stat2->set_expr(new SimpleConstAst(2));
    stat2->set_line(3);
    set_next(stat1, stat2);

    SimpleAssignmentAst *stat3 = new SimpleAssignmentAst();
    stat3->set_variable(var_z);
    stat3->set_expr(new SimpleConstAst(3));
    stat3->set_line(4);
    set_else_branch(stat3, condition);

    SimpleAssignmentAst *stat4 = new SimpleAssignmentAst();
    stat4->set_variable(var_a);
    stat4->set_expr(new SimpleConstAst(4));
    stat4->set_line(5);
    set_next(condition, stat4);

    SimpleRoot root(proc);
    SimpleSolverGenerator<NextSolver> *next_solver = 
        new SimpleSolverGenerator<NextSolver>(new NextSolver(root));

    NextSolver *next = next_solver->get_solver();

    ConditionSet condition_next;
    condition_next.insert(new SimpleStatementCondition(stat1));
    condition_next.insert(new SimpleStatementCondition(stat3));

    ConditionSet stat1_next;
    stat1_next.insert(new SimpleStatementCondition(stat2));

    ConditionSet stat23_next;
    stat23_next.insert(new SimpleStatementCondition(stat4));

    ConditionSet stat13_next;
    stat13_next.insert(new SimpleStatementCondition(stat2));
    stat13_next.insert(new SimpleStatementCondition(stat4));

    EXPECT_EQ(next->solve_right<StatementAst>(condition), condition_next);
    EXPECT_EQ(next->solve_right<StatementAst>(stat1), stat1_next);
    EXPECT_EQ(next->solve_right<StatementAst>(stat2), stat23_next);
    EXPECT_EQ(next->solve_right<StatementAst>(stat3), stat23_next);
    
    SimpleSolverGenerator<ModifiesSolver> *modifies_solver = 
        new SimpleSolverGenerator<ModifiesSolver>(new ModifiesSolver(root));

    ModifiesSolver *modifies = modifies_solver->get_solver();

    ConditionSet y_statements;
    y_statements.insert(new SimpleStatementCondition(stat2));
    y_statements.insert(new SimpleStatementCondition(condition));
    y_statements.insert(new SimpleProcCondition(proc));
    EXPECT_EQ(modifies->solve_left<SimpleVariable>(&var_y), y_statements);

    /*
     * Next(condition, S1)
     * Next(S1, S2)
     *
     * Get the next next statement after condition.
     */
    std::shared_ptr<SimplePredicate> pred(new SimpleWildCardPredicate(root));

    SimpleQueryMatcher next_matcher(next_solver);
    SimpleQueryVariable qvar_s1("S1", pred);
    SimpleQueryVariable qvar_s2("S2", pred);
    SimpleStatementCondition condition_condition(condition);

    next_matcher.solve_right(&condition_condition, &qvar_s1);
    EXPECT_EQ(qvar_s1.get_conditions(), condition_next);

    next_matcher.solve_both(&qvar_s1, &qvar_s2);
    EXPECT_EQ(qvar_s1.get_conditions(), condition_next);
    EXPECT_EQ(qvar_s2.get_conditions(), stat13_next);

    /*
     * Next(condition, T1)
     * Modifies(T2, "y")
     * Next(T1, T2)
     *
     * Get a statement that modifies variable y and is two-level
     * next to condition.
     */
    SimpleQueryMatcher modifies_matcher(modifies_solver);
    SimpleQueryVariable qvar_t1("T1", pred);
    SimpleQueryVariable qvar_t2("T2", pred);
    SimpleVariableCondition y_condition(var_y);

    ConditionSet t1_expected(new SimpleStatementCondition(stat1));
    ConditionSet t2_expected(new SimpleStatementCondition(stat2));

    next_matcher.solve_right(&condition_condition, &qvar_t1);
    EXPECT_EQ(qvar_t1.get_conditions(), condition_next);

    modifies_matcher.solve_left(&qvar_t2, &y_condition);
    EXPECT_EQ(qvar_t2.get_conditions(), y_statements);
    
    next_matcher.solve_both(&qvar_t1, &qvar_t2);
    EXPECT_EQ(qvar_t1.get_conditions(), t1_expected);
    EXPECT_EQ(qvar_t2.get_conditions(), t2_expected);
}


}
}
