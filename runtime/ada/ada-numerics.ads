-- Mathematical constants and the exception shared by the numerics children.
package Ada.Numerics is
    Argument_Error : exception;
    -- Keep exact literal fractions within the compiler's 127-bit capacity;
    -- these retain more precision than the supported floating-point types.
    Pi : constant := 3.14159_26535_89793_23846_26433_83279_50288;
    e  : constant := 2.71828_18284_59045_23536_02874_71352_66249;
end Ada.Numerics;
