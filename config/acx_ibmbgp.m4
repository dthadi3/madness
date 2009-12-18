AC_DEFUN([ACX_IBMBGP], [
        # If on an IBMBGP
        #   - defines HAVE_IBMBGP=1 in headers 
        #   - defines HAVE_IBMBGP=yes in the script
        #   - sets MPICXX=mpicxx and MPICC=mpicc if the user has not already set them
        #   - sets thread binding to "1 0 2"
        #   - enables spinlocks
        #   - sets the host architecture to powerpc-bgp-linux-gnu
        AC_CHECK_FILE([/bgsys],[HAVE_IBMBGP=yes AC_DEFINE([HAVE_IBMBGP],[1],[Defined if we are running on an IBMBGP])],[])
        if test "x$HAVE_IBMBGP" = xyes; then
                echo "IBM BG/P system detected"
                host="powerpc-bgp-linux"
                host_triplet="powerpc-bgp-linux"
                build="powerpc-bgp-linux"
                build_triplet="powerpc-bgp-linux"

                if test "x$MPICC" = x; then
                        echo "Choosing MPICC=mpicc for IBM BG/P"
                        MPICC=mpicc;
                fi
                if test "x$MPICXX" = x; then
                        echo "Choosing MPICXX=mpicxx for IBM BG/P"
                        MPICXX=mpicxx;
                fi
                BIND="1 0 2"
                AC_DEFINE(USE_SPINLOCKS, [1], [Define if should use spinlocks])
        fi
])




