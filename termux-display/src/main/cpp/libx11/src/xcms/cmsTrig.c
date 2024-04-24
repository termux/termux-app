
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
 * hereby granted to use, copy, modify, sell, and otherwise distribute this
 * software and its documentation for any purpose and without fee, provided
 * that this copyright, permission, and disclaimer notice is reproduced in
 * all copies of this software and in supporting documentation.  TekColor
 * is a trademark of Tektronix, Inc.
 *
 * Tektronix makes no representation about the suitability of this software
 * for any purpose.  It is provided "as is" and with all faults.
 *
 * TEKTRONIX DISCLAIMS ALL WARRANTIES APPLICABLE TO THIS SOFTWARE,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL TEKTRONIX BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR THE PERFORMANCE OF THIS SOFTWARE.
 */

/*
 *	It should be pointed out that for simplicity's sake, the
 *	environment parameters are defined as floating point constants,
 *	rather than octal or hexadecimal initializations of allocated
 *	storage areas.  This means that the range of allowed numbers
 *	may not exactly match the hardware's capabilities.  For example,
 *	if the maximum positive double precision floating point number
 *	is EXACTLY 1.11...E100 and the constant "MAXDOUBLE is
 *	defined to be 1.11E100 then the numbers between 1.11E100 and
 *	1.11...E100 are considered to be undefined.  For most
 *	applications, this will cause no problems.
 *
 *	An alternate method is to allocate a global static "double" variable,
 *	say "maxdouble", and use a union declaration and initialization
 *	to initialize it with the proper bits for the EXACT maximum value.
 *	This was not done because the only compilers available to the
 *	author did not fully support union initialization features.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xcmsint.h"

/* forward/static */
static double _XcmsModulo(double value, double base);
static double _XcmsPolynomial(
    register int order,
    double const *coeffs,
    double x);
static double
_XcmsModuloF(
    double val,
    register double *dp);

/*
 *	DEFINES
 */
#define XCMS_MAXERROR       	0.000001
#define XCMS_MAXITER       	10000
#define XCMS_PI       		3.14159265358979323846264338327950
#define XCMS_TWOPI 		6.28318530717958620
#define XCMS_HALFPI		1.57079632679489660
#define XCMS_FOURTHPI		0.785398163397448280
#define XCMS_SIXTHPI		0.523598775598298820
#define XCMS_RADIANS(d)		((d) * XCMS_PI / 180.0)
#define XCMS_DEGREES(r)		((r) * 180.0 / XCMS_PI)
#ifdef __vax__
#define XCMS_X6_UNDERFLOWS	(3.784659e-07)	/* X**6 almost underflows*/
#else
#define XCMS_X6_UNDERFLOWS	(4.209340e-52)	/* X**6 almost underflows */
#endif
#define XCMS_X16_UNDERFLOWS	(5.421010e-20)	/* X**16 almost underflows*/
#define XCMS_CHAR_BIT		8
#define XCMS_LONG_MAX		0x7FFFFFFF
#define XCMS_DEXPLEN		11
#define XCMS_NBITS(type)	(XCMS_CHAR_BIT * (int)sizeof(type))
#define XCMS_FABS(x)		((x) < 0.0 ? -(x) : (x))

/* XCMS_DMAXPOWTWO - largest power of two exactly representable as a double */
#define XCMS_DMAXPOWTWO	((double)(XCMS_LONG_MAX) * \
	    (1L << ((XCMS_NBITS(double)-XCMS_DEXPLEN) - XCMS_NBITS(int) + 1)))

/*
 *	LOCAL VARIABLES
 */

static double const cos_pcoeffs[] = {
    0.12905394659037374438e7,
   -0.37456703915723204710e6,
    0.13432300986539084285e5,
   -0.11231450823340933092e3
};

static double const cos_qcoeffs[] = {
    0.12905394659037373590e7,
    0.23467773107245835052e5,
    0.20969518196726306286e3,
    1.0
};

static double const sin_pcoeffs[] = {
    0.20664343336995858240e7,
   -0.18160398797407332550e6,
    0.35999306949636188317e4,
   -0.20107483294588615719e2
};

static double const sin_qcoeffs[] = {
    0.26310659102647698963e7,
    0.39270242774649000308e5,
    0.27811919481083844087e3,
    1.0
};

/*
 *
 *  FUNCTION
 *
 *	_XcmsCosine   double precision cosine
 *
 *  KEY WORDS
 *
 *	cos
 *	machine independent routines
 *	trigonometric functions
 *	math libraries
 *
 *  DESCRIPTION
 *
 *	Returns double precision cosine of double precision
 *	floating point argument.
 *
 *  USAGE
 *
 *	double _XcmsCosine (x)
 *	double x;
 *
 *  REFERENCES
 *
 *	Computer Approximations, J.F. Hart et al, John Wiley & Sons,
 *	1968, pp. 112-120.
 *
 *  RESTRICTIONS
 *
 *	The sin and cos routines are interactive in the sense that
 *	in the process of reducing the argument to the range -PI/4
 *	to PI/4, each may call the other.  Ultimately one or the
 *	other uses a polynomial approximation on the reduced
 *	argument.  The sin approximation has a maximum relative error
 *	of 10**(-17.59) and the cos approximation has a maximum
 *	relative error of 10**(-16.18).
 *
 *	These error bounds assume exact arithmetic
 *	in the polynomial evaluation.  Additional rounding and
 *	truncation errors may occur as the argument is reduced
 *	to the range over which the polynomial approximation
 *	is valid, and as the polynomial is evaluated using
 *	finite-precision arithmetic.
 *
 *  PROGRAMMER
 *
 *	Fred Fish
 *
 *  INTERNALS
 *
 *	Computes cos(x) from:
 *
 *		(1)	Reduce argument x to range -PI to PI.
 *
 *		(2)	If x > PI/2 then call cos recursively
 *			using relation cos(x) = -cos(x - PI).
 *
 *		(3)	If x < -PI/2 then call cos recursively
 *			using relation cos(x) = -cos(x + PI).
 *
 *		(4)	If x > PI/4 then call sin using
 *			relation cos(x) = sin(PI/2 - x).
 *
 *		(5)	If x < -PI/4 then call cos using
 *			relation cos(x) = sin(PI/2 + x).
 *
 *		(6)	If x would cause underflow in approx
 *			evaluation arithmetic then return
 *			sqrt(1.0 - x**2).
 *
 *		(7)	By now x has been reduced to range
 *			-PI/4 to PI/4 and the approximation
 *			from HART pg. 119 can be used:
 *
 *			cos(x) = ( p(y) / q(y) )
 *			Where:
 *
 *			y    =	x * (4/PI)
 *
 *			p(y) =  SUM [ Pj * (y**(2*j)) ]
 *			over j = {0,1,2,3}
 *
 *			q(y) =  SUM [ Qj * (y**(2*j)) ]
 *			over j = {0,1,2,3}
 *
 *			P0   =	0.12905394659037374438571854e+7
 *			P1   =	-0.3745670391572320471032359e+6
 *			P2   =	0.134323009865390842853673e+5
 *			P3   =	-0.112314508233409330923e+3
 *			Q0   =	0.12905394659037373590295914e+7
 *			Q1   =	0.234677731072458350524124e+5
 *			Q2   =	0.2096951819672630628621e+3
 *			Q3   =	1.0000...
 *			(coefficients from HART table #3843 pg 244)
 *
 *
 *	**** NOTE ****    The range reduction relations used in
 *	this routine depend on the final approximation being valid
 *	over the negative argument range in addition to the positive
 *	argument range.  The particular approximation chosen from
 *	HART satisfies this requirement, although not explicitly
 *	stated in the text.  This may not be true of other
 *	approximations given in the reference.
 *
 */

double _XcmsCosine(double x)
{
    auto double y;
    auto double yt2;
    double retval;

    if (x < -XCMS_PI || x > XCMS_PI) {
	x = _XcmsModulo (x, XCMS_TWOPI);
        if (x > XCMS_PI) {
	    x = x - XCMS_TWOPI;
        } else if (x < -XCMS_PI) {
	    x = x + XCMS_TWOPI;
        }
    }
    if (x > XCMS_HALFPI) {
	retval = -(_XcmsCosine (x - XCMS_PI));
    } else if (x < -XCMS_HALFPI) {
	retval = -(_XcmsCosine (x + XCMS_PI));
    } else if (x > XCMS_FOURTHPI) {
	retval = _XcmsSine (XCMS_HALFPI - x);
    } else if (x < -XCMS_FOURTHPI) {
	retval = _XcmsSine (XCMS_HALFPI + x);
    } else if (x < XCMS_X6_UNDERFLOWS && x > -XCMS_X6_UNDERFLOWS) {
	retval = _XcmsSquareRoot (1.0 - (x * x));
    } else {
	y = x / XCMS_FOURTHPI;
	yt2 = y * y;
	retval = _XcmsPolynomial (3, cos_pcoeffs, yt2) / _XcmsPolynomial (3, cos_qcoeffs, yt2);
    }
    return (retval);
}


/*
 *  FUNCTION
 *
 *	_XcmsModulo   double precision modulo
 *
 *  KEY WORDS
 *
 *	_XcmsModulo
 *	machine independent routines
 *	math libraries
 *
 *  DESCRIPTION
 *
 *	Returns double precision modulo of two double
 *	precision arguments.
 *
 *  USAGE
 *
 *	double _XcmsModulo (value, base)
 *	double value;
 *	double base;
 *
 *  PROGRAMMER
 *
 *	Fred Fish
 *
 */
static double _XcmsModulo(double value, double base)
{
    auto double intpart;

    value /= base;
    value = _XcmsModuloF (value, &intpart);
    value *= base;
    return(value);
}


/*
 * frac = (double) _XcmsModuloF(double val, double *dp)
 *	return fractional part of 'val'
 *	set *dp to integer part of 'val'
 *
 * Note  -> only compiled for the CA or KA.  For the KB/MC,
 * "math.c" instantiates a copy of the inline function
 * defined in "math.h".
 */
static double
_XcmsModuloF(
    double val,
    register double *dp)
{
	register double abs;
	/*
	 * Don't use a register for this.  The extra precision this results
	 * in on some systems causes problems.
	 */
	double ip;

	/* should check for illegal values here - nan, inf, etc */
	abs = XCMS_FABS(val);
	if (abs >= XCMS_DMAXPOWTWO) {
		ip = val;
	} else {
		ip = abs + XCMS_DMAXPOWTWO;	/* dump fraction */
		ip -= XCMS_DMAXPOWTWO;	/* restore w/o frac */
		if (ip > abs)		/* if it rounds up */
			ip -= 1.0;	/* fix it */
		ip = XCMS_FABS(ip);
	}
	*dp = ip;
	return (val - ip); /* signed fractional part */
}


/*
 *  FUNCTION
 *
 *	_XcmsPolynomial   double precision polynomial evaluation
 *
 *  KEY WORDS
 *
 *	poly
 *	machine independent routines
 *	math libraries
 *
 *  DESCRIPTION
 *
 *	Evaluates a polynomial and returns double precision
 *	result.  Is passed a the order of the polynomial,
 *	a pointer to an array of double precision polynomial
 *	coefficients (in ascending order), and the independent
 *	variable.
 *
 *  USAGE
 *
 *	double _XcmsPolynomial (order, coeffs, x)
 *	int order;
 *	double *coeffs;
 *	double x;
 *
 *  PROGRAMMER
 *
 *	Fred Fish
 *
 *  INTERNALS
 *
 *	Evalates the polynomial using recursion and the form:
 *
 *		P(x) = P0 + x(P1 + x(P2 +...x(Pn)))
 *
 */

static double _XcmsPolynomial(
    register int order,
    double const *coeffs,
    double x)
{
    auto double rtn_value;

    coeffs += order;
    rtn_value = *coeffs--;
    while(order-- > 0)
	rtn_value = *coeffs-- + (x * rtn_value);

    return(rtn_value);
}


/*
 *  FUNCTION
 *
 *	_XcmsSine	double precision sine
 *
 *  KEY WORDS
 *
 *	sin
 *	machine independent routines
 *	trigonometric functions
 *	math libraries
 *
 *  DESCRIPTION
 *
 *	Returns double precision sine of double precision
 *	floating point argument.
 *
 *  USAGE
 *
 *	double _XcmsSine (x)
 *	double x;
 *
 *  REFERENCES
 *
 *	Computer Approximations, J.F. Hart et al, John Wiley & Sons,
 *	1968, pp. 112-120.
 *
 *  RESTRICTIONS
 *
 *	The sin and cos routines are interactive in the sense that
 *	in the process of reducing the argument to the range -PI/4
 *	to PI/4, each may call the other.  Ultimately one or the
 *	other uses a polynomial approximation on the reduced
 *	argument.  The sin approximation has a maximum relative error
 *	of 10**(-17.59) and the cos approximation has a maximum
 *	relative error of 10**(-16.18).
 *
 *	These error bounds assume exact arithmetic
 *	in the polynomial evaluation.  Additional rounding and
 *	truncation errors may occur as the argument is reduced
 *	to the range over which the polynomial approximation
 *	is valid, and as the polynomial is evaluated using
 *	finite-precision arithmetic.
 *
 *  PROGRAMMER
 *
 *	Fred Fish
 *
 *  INTERNALS
 *
 *	Computes sin(x) from:
 *
 *	  (1)	Reduce argument x to range -PI to PI.
 *
 *	  (2)	If x > PI/2 then call sin recursively
 *		using relation sin(x) = -sin(x - PI).
 *
 *	  (3)	If x < -PI/2 then call sin recursively
 *		using relation sin(x) = -sin(x + PI).
 *
 *	  (4)	If x > PI/4 then call cos using
 *		relation sin(x) = cos(PI/2 - x).
 *
 *	  (5)	If x < -PI/4 then call cos using
 *		relation sin(x) = -cos(PI/2 + x).
 *
 *	  (6)	If x is small enough that polynomial
 *		evaluation would cause underflow
 *		then return x, since sin(x)
 *		approaches x as x approaches zero.
 *
 *	  (7)	By now x has been reduced to range
 *		-PI/4 to PI/4 and the approximation
 *		from HART pg. 118 can be used:
 *
 *		sin(x) = y * ( p(y) / q(y) )
 *		Where:
 *
 *		y    =  x * (4/PI)
 *
 *		p(y) =  SUM [ Pj * (y**(2*j)) ]
 *		over j = {0,1,2,3}
 *
 *		q(y) =  SUM [ Qj * (y**(2*j)) ]
 *		over j = {0,1,2,3}
 *
 *		P0   =  0.206643433369958582409167054e+7
 *		P1   =  -0.18160398797407332550219213e+6
 *		P2   =  0.359993069496361883172836e+4
 *		P3   =  -0.2010748329458861571949e+2
 *		Q0   =  0.263106591026476989637710307e+7
 *		Q1   =  0.3927024277464900030883986e+5
 *		Q2   =  0.27811919481083844087953e+3
 *		Q3   =  1.0000...
 *		(coefficients from HART table #3063 pg 234)
 *
 *
 *	**** NOTE ****	  The range reduction relations used in
 *	this routine depend on the final approximation being valid
 *	over the negative argument range in addition to the positive
 *	argument range.  The particular approximation chosen from
 *	HART satisfies this requirement, although not explicitly
 *	stated in the text.  This may not be true of other
 *	approximations given in the reference.
 *
 */

double
_XcmsSine (double x)
{
    double y;
    double yt2;
    double retval;

    if (x < -XCMS_PI || x > XCMS_PI) {
	x = _XcmsModulo (x, XCMS_TWOPI);
	if (x > XCMS_PI) {
	    x = x - XCMS_TWOPI;
	} else if (x < -XCMS_PI) {
	    x = x + XCMS_TWOPI;
	}
    }
    if (x > XCMS_HALFPI) {
	retval = -(_XcmsSine (x - XCMS_PI));
    } else if (x < -XCMS_HALFPI) {
	retval = -(_XcmsSine (x + XCMS_PI));
    } else if (x > XCMS_FOURTHPI) {
	retval = _XcmsCosine (XCMS_HALFPI - x);
    } else if (x < -XCMS_FOURTHPI) {
	retval = -(_XcmsCosine (XCMS_HALFPI + x));
    } else if (x < XCMS_X6_UNDERFLOWS && x > -XCMS_X6_UNDERFLOWS) {
	retval = x;
    } else {
	y = x / XCMS_FOURTHPI;
	yt2 = y * y;
	retval = y * (_XcmsPolynomial (3, sin_pcoeffs, yt2) / _XcmsPolynomial(3, sin_qcoeffs, yt2));
    }
    return(retval);
}


/*
 *	NAME
 *		_XcmsArcTangent
 *
 *	SYNOPSIS
 */
double
_XcmsArcTangent(double x)
/*
 *	DESCRIPTION
 *		Computes the arctangent.
 *		This is an implementation of the Gauss algorithm as
 *		described in:
 *		    Forman S. Acton, Numerical Methods That Work,
 *			New York, NY, Harper & Row, 1970.
 *
 *	RETURNS
 *		Returns the arctangent
 */
{
    double ai, a1 = 0.0, bi, b1 = 0.0, l, d;
    double maxerror;
    int i;

    if (x == 0.0)  {
	return (0.0);
    }
    if (x < 1.0) {
	maxerror = x * XCMS_MAXERROR;
    } else {
	maxerror = XCMS_MAXERROR;
    }
    ai = _XcmsSquareRoot( 1.0 / (1.0 + (x * x)) );
    bi = 1.0;
    for (i = 0; i < XCMS_MAXITER; i++) {
	a1 = (ai + bi) / 2.0;
	b1 = _XcmsSquareRoot((a1 * bi));
	if (a1 == b1)
	    break;
	d = XCMS_FABS(a1 - b1);
	if (d < maxerror)
	    break;
	ai = a1;
	bi = b1;
    }

    l = ((a1 > b1) ? b1 : a1);

    a1 = _XcmsSquareRoot(1 + (x * x));
    return (x / (a1 * l));
}
