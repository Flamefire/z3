#include "math/polysat/slicing.h"
#include "math/polysat/solver.h"

namespace polysat {

    struct solver_scope_slicing {
        reslimit lim;
    };

    class scoped_solver_slicing : public solver_scope_slicing, public solver {
    public:
        scoped_solver_slicing(): solver(lim) {}
        slicing& sl() { return m_slicing; }
    };

    class test_slicing {
    public:

        // x[7:3] = a
        // y[5:0] = b
        // x = y
        static void test1() {
            std::cout << __func__ << "\n";
            scoped_solver_slicing s;
            slicing& sl = s.sl();
            pvar x = s.add_var(8);
            pvar y = s.add_var(8);
            pvar a = s.add_var(5);
            pvar b = s.add_var(6);

            std::cout << sl << "\n";
            slicing::enode_vector x_7_3;
            sl.mk_slice(sl.var2slice(x), 7, 3, x_7_3);
            std::cout << sl << "\n";
            slicing::enode_vector a_4_0;
            sl.mk_slice(sl.var2slice(a), 4, 0, a_4_0);
            std::cout << sl << "\n";
            VERIFY(sl.merge(x_7_3, a_4_0, sat::literal(1)));
            std::cout << sl << "\n";

            slicing::enode_vector y_5_0;
            sl.mk_slice(sl.var2slice(y), 5, 0, y_5_0);
            VERIFY(sl.merge(y_5_0, sl.var2slice(b), sat::literal(2)));
            std::cout << sl << "\n";

            slicing::enode_vector x_base;
            sl.get_root_base(sl.var2slice(x), x_base);
            slicing::enode_vector y_base;
            sl.get_root_base(sl.var2slice(y), y_base);
            VERIFY(sl.merge(x_base, y_base, sat::literal(3)));
            std::cout << sl << "\n";

            sl.display_tree(std::cout);
        }

        // x[7:3] = a
        // x = y
        // y[5:0] = b
        static void test2() {
            std::cout << __func__ << "\n";
            scoped_solver_slicing s;
            slicing& sl = s.sl();
            pvar x = s.add_var(8);
            pvar y = s.add_var(8);

            pvar a = sl.mk_extract_var(x, 7, 3);
            std::cout << sl << "\n";

            VERIFY(sl.merge(sl.var2slice(x), sl.var2slice(y), sat::literal(1)));
            std::cout << sl << "\n";

            pvar b = sl.mk_extract_var(y, 5, 0);
            std::cout << sl << "\n";

            sl.display_tree(std::cout);

            (void)a;
            (void)b;
        }

        // x[7:3] = a
        // y[5:0] = b
        // x[5:0] = c
        // x[5:4] ++ y[3:0] = d
        // x = y
        //
        // How easily can we find b=c and b=d?
        static void test3() {
            std::cout << __func__ << "\n";
            scoped_solver_slicing s;
            slicing& sl = s.sl();
            pvar x = s.add_var(8);
            pvar y = s.add_var(8);
            std::cout << sl << "\n";

            pvar a = sl.mk_extract_var(x, 7, 3);
            std::cout << "v" << a << " := v" << x << "[7:3]\n" << sl << "\n";
            pvar b = sl.mk_extract_var(y, 5, 0);
            std::cout << "v" << b << " := v" << y << "[5:0]\n" << sl << "\n";
            pvar c = sl.mk_extract_var(x, 5, 0);
            std::cout << "v" << c << " := v" << x << "[5:0]\n" << sl << "\n";
            pdd d = sl.mk_concat(sl.mk_extract(x, 5, 4), sl.mk_extract(y, 3, 0));
            std::cout << d << " := v" << x << "[5:4] ++ v" << y << "[3:0]\n" << sl << "\n";

            std::cout << "v" << b << " = v" << c << "? " << sl.is_equal(sl.var2slice(b), sl.var2slice(c)) << "\n\n";
            std::cout << "v" << b << " = "  << d << "? " << sl.is_equal(sl.var2slice(b), sl.pdd2slice(d)) << "\n\n";

            VERIFY(sl.merge(sl.var2slice(x), sl.var2slice(y), sat::literal(123)));
            std::cout << "v" << x << " = v" << y << "\n" << sl << "\n";

            std::cout << "v" << b << " = v" << c << "? " << sl.is_equal(sl.var2slice(b), sl.var2slice(c))
                      << "    root(v" << b << ") = " << sl.var2slice(b)->get_root_id()
                      << "    root(v" << c << ") = " << sl.var2slice(c)->get_root_id()
                      << "\n";
            sat::literal_vector reason;
            sl.explain_equal(sl.var2slice(b), sl.var2slice(c), reason);
            std::cout << "    Reason: " << reason << "\n\n";

            std::cout << "v" << b << " = "  << d << "? " << sl.is_equal(sl.var2slice(b), sl.pdd2slice(d))
                      << "    root(v" << b << ") = " << sl.var2slice(b)->get_root_id()
                      << "    root("  << d << ") = " << sl.pdd2slice(d)->get_root_id()
                      << "\n";
            reason.reset();
            sl.explain_equal(sl.var2slice(b), sl.pdd2slice(d), reason);
            std::cout << "    Reason: " << reason << "\n\n";
        }

        // 1. a = b
        // 2. d = c[1:0]
        // 3. c = b[3:0]
        // 4. e = a[1:0]
        //
        // Explain(d = e) should be {1, 2, 3, 4}
        static void test4() {
            std::cout << __func__ << "\n";
            scoped_solver_slicing s;
            slicing& sl = s.sl();
            pvar a = s.add_var(8);
            pvar b = s.add_var(8);
            pvar c = s.add_var(4);
            pvar d = s.add_var(2);
            pvar e = s.add_var(2);

            VERIFY(sl.merge(sl.var2slice(a), sl.var2slice(b), sat::literal(101)));
            VERIFY(sl.merge(sl.var2slice(d), sl.var2slice(sl.mk_extract_var(c, 1, 0)), sat::literal(102)));
            VERIFY(sl.merge(sl.var2slice(c), sl.var2slice(sl.mk_extract_var(b, 3, 0)), sat::literal(103)));
            VERIFY(sl.merge(sl.var2slice(e), sl.var2slice(sl.mk_extract_var(a, 1, 0)), sat::literal(104)));

            std::cout << "v" << d << " = v" << e << "? " << sl.is_equal(sl.var2slice(d), sl.var2slice(e))
                      << "    root(v" << d << ") = " << sl.var2slice(d)->get_root_id()
                      << "    root(v" << e << ") = " << sl.var2slice(e)->get_root_id()
                      << "    slice(v" << d << ") = " << sl.var2slice(d)->get_id()
                      << "    slice(v" << e << ") = " << sl.var2slice(e)->get_id()
                      << "\n";
            sat::literal_vector reason;
            sl.explain_equal(sl.var2slice(d), sl.var2slice(e), reason);
            std::cout << "    Reason: " << reason << "\n";
            sl.display_tree(std::cout);
        }

        // x[5:2] = y
        // x[3:0] = z
        // y = 0b1001
        // z = 0b0111
        static void test5() {
            std::cout << __func__ << "\n";
            scoped_solver_slicing s;
            slicing& sl = s.sl();
            pvar x = s.add_var(6);
            std::cout << sl << "\n";

            pvar y = sl.mk_extract_var(x, 5, 2);
            std::cout << "v" << y << " := v" << x << "[5:2]\n" << sl << "\n";
            pvar z = sl.mk_extract_var(x, 3, 0);
            std::cout << "v" << z << " := v" << x << "[3:0]\n" << sl << "\n";

            // VERIFY(sl.merge_value(sl.var2slice(y), rational(9)));
            // std::cout << "v" << y << " = 9\n" << sl << "\n";
            // VERIFY(sl.merge_value(sl.var2slice(z), rational(7)));
            // std::cout << "v" << z << " = 7\n" << sl << "\n";
        }

    };

}


void tst_slicing() {
    using namespace polysat;
    test_slicing::test1();
    test_slicing::test2();
    test_slicing::test3();
    test_slicing::test4();
    // test_slicing::test5();
    std::cout << "ok\n";
}
