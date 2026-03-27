/* dos-cwi.h

   Standard definitions used by all my code

 9 February 1997

*/

#ifndef	DOS_CWI_H			/* Double-include control here */
#define DOS_CWI_H

// Types for Microsoft C++ DOS compiler

typedef	unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned long		u32;

typedef char			i8;
typedef short			i16;
typedef long			i32;

typedef	float			f32;
typedef	double			f64;
typedef	long double		f128;  // I'm not sure this works for MSDOS C++ compiler

typedef int			error_indicator;/* A type for functions,
						 usually... */
typedef unsigned int		boolean;	/* "natural" type */
typedef char			c8;		/* 8-bit character */
typedef unsigned char		uc8;		/* 8-bit character, unsigned*/
						/* it is sometimes useful to*/
						/* make the distinction*/
// Probably not required, but ...

#ifdef	FALSE
#undef	FALSE
#endif
#define	FALSE	(0)

#ifdef	TRUE
#undef	TRUE
#endif
#define	TRUE	(!FALSE)


// I find this small syntax tweak helps understand 'case' statements
// and reduced 'fall through' errors

#define	is	:{
#define	esac	break;}

#endif	/* of check if DOS_CWI_H was defined*/
