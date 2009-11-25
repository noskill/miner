(ConceptNode "symmetricRelation" (stv 1 0))
(ConceptNode "transitiveRelation" (stv 1 0))
(ConceptNode "Britney2" (stv 1 0))
(ConceptNode "Osama2" (stv 1 0))
(ConceptNode "x014" (stv 1 0))
(ConceptNode "x012" (stv 0.050000001 0.99000001))
(ConceptNode "R1" (stv 0.92000002 0))
(ConceptNode "___PLN___")
(ConceptNode "X1" (stv 1 0))
(ConceptNode "Osama" (stv 0.001 0.99900001))
(ConceptNode "Abu" (stv 9.9999997e-06 0.99900001))
(ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))
(ConceptNode "Britney" (stv 9.9999997e-06 0.99900001))
(ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))
(ConceptNode "Judea" (stv 9.9999997e-06 0.99900001))
(ConceptNode "Muhammad" (stv 9.9999997e-06 0.99900001))
(ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001))
(ConceptNode "killer" (stv 0.050000001 0.99900001))
(ConceptNode "terrorist" (stv 0.0099999998 0.99900001))
(ListLink (stv 1 0) (ConceptNode "Britney2" (stv 1 0))
    (ConceptNode "Osama2" (stv 1 0)))
(ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "x014" (stv 1 0)))
(ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))
    (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001)))
(ListLink (stv 1 0) (ConceptNode "x012" (stv 0.050000001 0.99000001))
    (ConceptNode "Osama" (stv 0.001 0.99900001)))
(ListLink (stv 1 0) (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "Osama" (stv 0.001 0.99900001)))
(ListLink (stv 1 0) (ConceptNode "Muhammad" (stv 9.9999997e-06 0.99900001)))
(ListLink (stv 1 0) (ConceptNode "X1" (stv 1 0)))
(ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001)))
(ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
    (VariableNode "X008" (stv 1 0))
    (VariableNode "R000" (stv 1 0)))
(ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
    (VariableNode "X008" (stv 1 0)))
(ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "Britney" (stv 9.9999997e-06 0.99900001)))
(ListLink (stv 1 0) (VariableNode "X008" (stv 1 0))
    (VariableNode "X007" (stv 1 0)))
(ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001)))
(ListLink (stv 1 0) (ConceptNode "Britney" (stv 9.9999997e-06 0.99900001)))
(ListLink (stv 1 0) (ConceptNode "x014" (stv 1 0)))
(AndLink (stv 1 0) (InheritanceLink (stv 1 0) (VariableNode "R000" (stv 1 0))
    (ConceptNode "symmetricRelation" (stv 1 0)))
    (EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
       (ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
          (VariableNode "X008" (stv 1 0)))))
(AndLink (stv 1 0) (EvaluationLink (stv 1 0) (PredicateNode "wasKilled" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "x014" (stv 1 0))))
    (EvaluationLink (stv 1 0) (PredicateNode "friendOf" (stv 1 0))
       (ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))
          (ConceptNode "x014" (stv 1 0)))))
(VariableNode "R000" (stv 1 0))
(VariableNode "X007" (stv 1 0))
(VariableNode "X008" (stv 1 0))
(ForAllLink (stv 0.99000001 0.99000001) (ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
    (VariableNode "X008" (stv 1 0))
    (VariableNode "R000" (stv 1 0)))
    (ImplicationLink (stv 1 0) (AndLink (stv 1 0) (InheritanceLink (stv 1 0) (VariableNode "R000" (stv 1 0))
    (ConceptNode "symmetricRelation" (stv 1 0)))
    (EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
       (ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
          (VariableNode "X008" (stv 1 0)))))
       (EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
          (ListLink (stv 1 0) (VariableNode "X008" (stv 1 0))
             (VariableNode "X007" (stv 1 0))))))
(ImplicationLink (stv 1 0.99900001) (EvaluationLink (stv 1 0.25) (PredicateNode "test3" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))))
    (EvaluationLink (stv 1 0) (PredicateNode "test2" (stv 1 0))
       (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001)))))
(ImplicationLink (stv 0.99000001 0.99000001) (PredicateNode "wasKilled" (stv 1 0))
    (PredicateNode "isDead" (stv 1 0)))
(ImplicationLink (stv 1 0) (AndLink (stv 1 0) (InheritanceLink (stv 1 0) (VariableNode "R000" (stv 1 0))
    (ConceptNode "symmetricRelation" (stv 1 0)))
    (EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
       (ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
          (VariableNode "X008" (stv 1 0)))))
    (EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
       (ListLink (stv 1 0) (VariableNode "X008" (stv 1 0))
          (VariableNode "X007" (stv 1 0)))))
(EvaluationLink (stv 0.80000001 0.2) (PredicateNode "friendOf" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Britney2" (stv 1 0))
       (ConceptNode "Osama2" (stv 1 0))))
(EvaluationLink (stv 1 0) (PredicateNode "friendOf" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))
       (ConceptNode "x014" (stv 1 0))))
(EvaluationLink (stv 0.80000001 0) (PredicateNode "killerOf" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))
       (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))))
(EvaluationLink (stv 1 0) (PredicateNode "friendOf" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "x012" (stv 0.050000001 0.99000001))
       (ConceptNode "Osama" (stv 0.001 0.99900001))))
(EvaluationLink (stv 0.99000001 0.80000001) (PredicateNode "killed" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))
       (ConceptNode "Osama" (stv 0.001 0.99900001))))
(EvaluationLink (stv 1 0) (PredicateNode "wasKilled" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Muhammad" (stv 9.9999997e-06 0.99900001))))
(EvaluationLink (stv 0.87 0) (ConceptNode "R1" (stv 0.92000002 0))
    (ListLink (stv 1 0) (ConceptNode "X1" (stv 1 0))))
(EvaluationLink (stv 1 0) (PredicateNode "isDead" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))))
(EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
    (ListLink (stv 1 0) (VariableNode "X007" (stv 1 0))
       (VariableNode "X008" (stv 1 0))))
(EvaluationLink (stv 0.80000001 0.40000001) (PredicateNode "friendOf" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Amir" (stv 9.9999997e-06 0.99900001))
       (ConceptNode "Britney" (stv 9.9999997e-06 0.99900001))))
(EvaluationLink (stv 1 0) (VariableNode "R000" (stv 1 0))
    (ListLink (stv 1 0) (VariableNode "X008" (stv 1 0))
       (VariableNode "X007" (stv 1 0))))
(EvaluationLink (stv 1 0.99900001) (PredicateNode "test1" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))))
(EvaluationLink (stv 0.2 0.60000002) (PredicateNode "isDead" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Britney" (stv 9.9999997e-06 0.99900001))))
(EvaluationLink (stv 1 0.25) (PredicateNode "test3" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))))
(EvaluationLink (stv 1 0) (PredicateNode "test2" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))))
(EvaluationLink (stv 1 0) (PredicateNode "wasKilled" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "x014" (stv 1 0))))
(InheritanceLink (stv 1 0) (VariableNode "R000" (stv 1 0))
    (ConceptNode "symmetricRelation" (stv 1 0)))
(InheritanceLink (stv 0.99000001 0.99000001) (ConceptNode "Judea" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "killer" (stv 0.050000001 0.99900001)))
(InheritanceLink (stv 0.99900001 0.99900001) (PredicateNode "friendOf" (stv 1 0))
    (ConceptNode "symmetricRelation" (stv 1 0)))
(InheritanceLink (stv 0.94999999 0.99000001) (ConceptNode "Muhammad" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001)))
(InheritanceLink (stv 0.99000001 0.99000001) (PredicateNode "friendOf" (stv 1 0))
    (ConceptNode "transitiveRelation" (stv 1 0)))
(InheritanceLink (stv 0.94999999 0.99000001) (ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001))
    (ConceptNode "Osama" (stv 0.001 0.99900001)))
(InheritanceLink (stv 0.98000002 0.99000001) (ConceptNode "Osama" (stv 0.001 0.99900001))
    (ConceptNode "terrorist" (stv 0.0099999998 0.99900001)))
(InheritanceLink (stv 0.94999999 0.99000001) (ConceptNode "Abu" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "terrorist" (stv 0.0099999998 0.99900001)))
(InheritanceLink (stv 0.99000001 0.99000001) (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "killer" (stv 0.050000001 0.99900001)))
(InheritanceLink (stv 0.89999998 0) (EvaluationLink (stv 0.80000001 0) (PredicateNode "killerOf" (stv 1 0))
    (ListLink (stv 1 0) (ConceptNode "Osama" (stv 0.001 0.99900001))
       (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))))
    (EvaluationLink (stv 0.99000001 0.80000001) (PredicateNode "killed" (stv 1 0))
       (ListLink (stv 1 0) (ConceptNode "Moses" (stv 9.9999997e-06 0.99900001))
          (ConceptNode "Osama" (stv 0.001 0.99900001)))))
(ExtensionalInheritanceLink (stv 0.99000001 0.99000001) (ConceptNode "Osama" (stv 0.001 0.99900001))
    (ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001)))
(ExtensionalInheritanceLink (stv 0.99000001 0.99000001) (ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001))
    (ConceptNode "Abu" (stv 9.9999997e-06 0.99900001)))
(ExtensionalInheritanceLink (stv 0.99000001 0.99000001) (ConceptNode "Abu" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001)))
(ExtensionalInheritanceLink (stv 0.99000001 0.99000001) (ConceptNode "Muhammad" (stv 9.9999997e-06 0.99900001))
    (ConceptNode "AlQaeda" (stv 9.9999997e-05 0.99900001)))
(PredicateNode "test1" (stv 1 0))
(PredicateNode "test3" (stv 1 0))
(PredicateNode "wasKilled" (stv 1 0))
(PredicateNode "test2" (stv 1 0))
(PredicateNode "isDead" (stv 1 0))
(PredicateNode "friendOf" (stv 1 0))
(PredicateNode "pastForm" (stv 1 0))
(PredicateNode "kill" (stv 1 0))
(PredicateNode "killed" (stv 1 0))
(PredicateNode "killerOf" (stv 1 0))
(ExecutionOutputLink (stv 0.99000001 0.99000001) (PredicateNode "pastForm" (stv 1 0))
    (PredicateNode "kill" (stv 1 0)))
(EquivalenceLink (stv 1 0) (PredicateNode "killed" (stv 1 0))
    (ExecutionOutputLink (stv 0.99000001 0.99000001) (PredicateNode "pastForm" (stv 1 0))
       (PredicateNode "kill" (stv 1 0))))
