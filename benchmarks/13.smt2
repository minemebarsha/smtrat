(set-logic QF_NRA)
(declare-fun t () Real)
(declare-fun x () Real)
(declare-fun y () Real)
(assert (and (>= (- (* 17 t) 96) 0) (<= (- (* 17 t) 160) 0) (>= (+ (- (* 16 x) (* 17 t)) 16) 0) (<= (- (- (* 16 x) (* 17 t)) 16) 0) (>= (+ (- (* 16 y) (* 17 t)) 144) 0) (<= (+ (- (* 16 y) (* 17 t)) 112) 0) (<= (- (+ (* (- x t) (- x t)) (* y y)) 1) 0)))
(eliminate-quantifiers (exists t x y))
(exit)
