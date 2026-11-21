with Ada.Numerics;
with Interfaces.C;

package body Ada.Numerics.Generic_Elementary_Functions is

    subtype C_Double is Interfaces.C.double;
    subtype C_Single is Interfaces.C.C_float;

    -- Size selects the actual machine representation, including for constrained
    -- actual subtypes. QBE removes the unused branch of this static selection.

    function C_Sqrt (X : C_Double) return C_Double;
    pragma Import (C, C_Sqrt, "__ada_numerics_sqrt");

    function C_Log (X : C_Double) return C_Double;
    pragma Import (C, C_Log, "__ada_numerics_log");

    function C_Exp (X : C_Double) return C_Double;
    pragma Import (C, C_Exp, "__ada_numerics_exp");

    function C_Sin (X : C_Double) return C_Double;
    pragma Import (C, C_Sin, "__ada_numerics_sin");

    function C_Cos (X : C_Double) return C_Double;
    pragma Import (C, C_Cos, "__ada_numerics_cos");

    function C_Tan (X : C_Double) return C_Double;
    pragma Import (C, C_Tan, "__ada_numerics_tan");

    function C_Cot (X : C_Double) return C_Double;
    pragma Import (C, C_Cot, "__ada_numerics_cot");

    function C_Arcsin (X : C_Double) return C_Double;
    pragma Import (C, C_Arcsin, "__ada_numerics_arcsin");

    function C_Arccos (X : C_Double) return C_Double;
    pragma Import (C, C_Arccos, "__ada_numerics_arccos");

    function C_Sinh (X : C_Double) return C_Double;
    pragma Import (C, C_Sinh, "__ada_numerics_sinh");

    function C_Cosh (X : C_Double) return C_Double;
    pragma Import (C, C_Cosh, "__ada_numerics_cosh");

    function C_Tanh (X : C_Double) return C_Double;
    pragma Import (C, C_Tanh, "__ada_numerics_tanh");

    function C_Coth (X : C_Double) return C_Double;
    pragma Import (C, C_Coth, "__ada_numerics_coth");

    function C_Arcsinh (X : C_Double) return C_Double;
    pragma Import (C, C_Arcsinh, "__ada_numerics_arcsinh");

    function C_Arccosh (X : C_Double) return C_Double;
    pragma Import (C, C_Arccosh, "__ada_numerics_arccosh");

    function C_Arctanh (X : C_Double) return C_Double;
    pragma Import (C, C_Arctanh, "__ada_numerics_arctanh");

    function C_Arccoth (X : C_Double) return C_Double;
    pragma Import (C, C_Arccoth, "__ada_numerics_arccoth");

    function C_Log_Base (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Log_Base, "__ada_numerics_log_base");

    function C_Power (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Power, "__ada_numerics_power");

    function C_Sin_Cycle (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Sin_Cycle, "__ada_numerics_sin_cycle");

    function C_Cos_Cycle (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Cos_Cycle, "__ada_numerics_cos_cycle");

    function C_Tan_Cycle (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Tan_Cycle, "__ada_numerics_tan_cycle");

    function C_Cot_Cycle (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Cot_Cycle, "__ada_numerics_cot_cycle");

    function C_Arcsin_Cycle (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Arcsin_Cycle, "__ada_numerics_arcsin_cycle");

    function C_Arccos_Cycle (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Arccos_Cycle, "__ada_numerics_arccos_cycle");

    function C_Arctan (X, Y : C_Double) return C_Double;
    pragma Import (C, C_Arctan, "__ada_numerics_arctan");

    function C_Arctan_Cycle (X, Y, Cycle : C_Double) return C_Double;
    pragma Import (C, C_Arctan_Cycle, "__ada_numerics_arctan_cycle");

    function C_Sqrt_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Sqrt_32, "__ada_numerics_sqrt_f32");

    function C_Log_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Log_32, "__ada_numerics_log_f32");

    function C_Exp_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Exp_32, "__ada_numerics_exp_f32");

    function C_Sin_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Sin_32, "__ada_numerics_sin_f32");

    function C_Cos_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Cos_32, "__ada_numerics_cos_f32");

    function C_Tan_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Tan_32, "__ada_numerics_tan_f32");

    function C_Cot_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Cot_32, "__ada_numerics_cot_f32");

    function C_Arcsin_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Arcsin_32, "__ada_numerics_arcsin_f32");

    function C_Arccos_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Arccos_32, "__ada_numerics_arccos_f32");

    function C_Sinh_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Sinh_32, "__ada_numerics_sinh_f32");

    function C_Cosh_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Cosh_32, "__ada_numerics_cosh_f32");

    function C_Tanh_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Tanh_32, "__ada_numerics_tanh_f32");

    function C_Coth_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Coth_32, "__ada_numerics_coth_f32");

    function C_Arcsinh_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Arcsinh_32, "__ada_numerics_arcsinh_f32");

    function C_Arccosh_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Arccosh_32, "__ada_numerics_arccosh_f32");

    function C_Arctanh_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Arctanh_32, "__ada_numerics_arctanh_f32");

    function C_Arccoth_32 (X : C_Single) return C_Single;
    pragma Import (C, C_Arccoth_32, "__ada_numerics_arccoth_f32");

    function C_Log_Base_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Log_Base_32, "__ada_numerics_log_base_f32");

    function C_Power_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Power_32, "__ada_numerics_power_f32");

    function C_Sin_Cycle_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Sin_Cycle_32, "__ada_numerics_sin_cycle_f32");

    function C_Cos_Cycle_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Cos_Cycle_32, "__ada_numerics_cos_cycle_f32");

    function C_Tan_Cycle_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Tan_Cycle_32, "__ada_numerics_tan_cycle_f32");

    function C_Cot_Cycle_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Cot_Cycle_32, "__ada_numerics_cot_cycle_f32");

    function C_Arcsin_Cycle_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Arcsin_Cycle_32, "__ada_numerics_arcsin_cycle_f32");

    function C_Arccos_Cycle_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Arccos_Cycle_32, "__ada_numerics_arccos_cycle_f32");

    function C_Arctan_32 (X, Y : C_Single) return C_Single;
    pragma Import (C, C_Arctan_32, "__ada_numerics_arctan_f32");

    function C_Arctan_Cycle_32 (X, Y, Cycle : C_Single) return C_Single;
    pragma Import (C, C_Arctan_Cycle_32, "__ada_numerics_arctan_cycle_f32");

    function Sqrt (X : Float_Type'Base) return Float_Type'Base is
    begin
        if X < 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Sqrt_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Sqrt (C_Double (X)));
        end if;
    end Sqrt;

    function Log (X : Float_Type'Base) return Float_Type'Base is
    begin
        if X < 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Log_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Log (C_Double (X)));
        end if;
    end Log;

    function Exp (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Exp_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Exp (C_Double (X)));
        end if;
    end Exp;

    function Sin (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Sin_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Sin (C_Double (X)));
        end if;
    end Sin;

    function Cos (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Cos_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Cos (C_Double (X)));
        end if;
    end Cos;

    function Tan (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Tan_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Tan (C_Double (X)));
        end if;
    end Tan;

    function Cot (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Cot_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Cot (C_Double (X)));
        end if;
    end Cot;

    function Arcsin (X : Float_Type'Base) return Float_Type'Base is
    begin
        if abs X > 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arcsin_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Arcsin (C_Double (X)));
        end if;
    end Arcsin;

    function Arccos (X : Float_Type'Base) return Float_Type'Base is
    begin
        if abs X > 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arccos_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Arccos (C_Double (X)));
        end if;
    end Arccos;

    function Sinh (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Sinh_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Sinh (C_Double (X)));
        end if;
    end Sinh;

    function Cosh (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Cosh_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Cosh (C_Double (X)));
        end if;
    end Cosh;

    function Tanh (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Tanh_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Tanh (C_Double (X)));
        end if;
    end Tanh;

    function Coth (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Coth_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Coth (C_Double (X)));
        end if;
    end Coth;

    function Arcsinh (X : Float_Type'Base) return Float_Type'Base is
    begin
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arcsinh_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Arcsinh (C_Double (X)));
        end if;
    end Arcsinh;

    function Arccosh (X : Float_Type'Base) return Float_Type'Base is
    begin
        if X < 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arccosh_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Arccosh (C_Double (X)));
        end if;
    end Arccosh;

    function Arctanh (X : Float_Type'Base) return Float_Type'Base is
    begin
        if abs X > 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arctanh_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Arctanh (C_Double (X)));
        end if;
    end Arctanh;

    function Arccoth (X : Float_Type'Base) return Float_Type'Base is
    begin
        if abs X < 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arccoth_32 (C_Single (X)));
        else
            return Float_Type'Base (C_Arccoth (C_Double (X)));
        end if;
    end Arccoth;

    function Log (X, Base : Float_Type'Base) return Float_Type'Base is
    begin
        if X < 0.0 or Base <= 0.0 or Base = 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Log_Base_32 (C_Single (X), C_Single (Base)));
        else
            return Float_Type'Base (C_Log_Base (C_Double (X), C_Double (Base)));
        end if;
    end Log;

    function "**" (Left, Right : Float_Type'Base) return Float_Type'Base is
    begin
        if Left < 0.0 or (Left = 0.0 and Right = 0.0) then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Power_32 (C_Single (Left), C_Single (Right)));
        else
            return Float_Type'Base (C_Power (C_Double (Left), C_Double (Right)));
        end if;
    end "**";

    function Sin (X, Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Sin_Cycle_32 (C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Sin_Cycle (C_Double (X), C_Double (Cycle)));
        end if;
    end Sin;

    function Cos (X, Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Cos_Cycle_32 (C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Cos_Cycle (C_Double (X), C_Double (Cycle)));
        end if;
    end Cos;

    function Tan (X, Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Tan_Cycle_32 (C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Tan_Cycle (C_Double (X), C_Double (Cycle)));
        end if;
    end Tan;

    function Cot (X, Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Cot_Cycle_32 (C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Cot_Cycle (C_Double (X), C_Double (Cycle)));
        end if;
    end Cot;

    function Arcsin (X, Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 or abs X > 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arcsin_Cycle_32 (C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Arcsin_Cycle (C_Double (X), C_Double (Cycle)));
        end if;
    end Arcsin;

    function Arccos (X, Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 or abs X > 1.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arccos_Cycle_32 (C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Arccos_Cycle (C_Double (X), C_Double (Cycle)));
        end if;
    end Arccos;

    function Arctan (Y : Float_Type'Base; X : Float_Type'Base := 1.0) return Float_Type'Base is
    begin
        if X = 0.0 and Y = 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arctan_32 (C_Single (Y), C_Single (X)));
        else
            return Float_Type'Base (C_Arctan (C_Double (Y), C_Double (X)));
        end if;
    end Arctan;

    function Arctan (Y : Float_Type'Base; X : Float_Type'Base := 1.0; Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 or (X = 0.0 and Y = 0.0) then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arctan_Cycle_32 (C_Single (Y), C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Arctan_Cycle (C_Double (Y), C_Double (X), C_Double (Cycle)));
        end if;
    end Arctan;

    function Arccot (X : Float_Type'Base; Y : Float_Type'Base := 1.0) return Float_Type'Base is
    begin
        if X = 0.0 and Y = 0.0 then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arctan_32 (C_Single (Y), C_Single (X)));
        else
            return Float_Type'Base (C_Arctan (C_Double (Y), C_Double (X)));
        end if;
    end Arccot;

    function Arccot (X : Float_Type'Base; Y : Float_Type'Base := 1.0; Cycle : Float_Type'Base) return Float_Type'Base is
    begin
        if Cycle <= 0.0 or (X = 0.0 and Y = 0.0) then
            raise Ada.Numerics.Argument_Error;
        end if;
        if Float_Type'Base'Size = C_Single'Size then
            return Float_Type'Base (C_Arctan_Cycle_32 (C_Single (Y), C_Single (X), C_Single (Cycle)));
        else
            return Float_Type'Base (C_Arctan_Cycle (C_Double (Y), C_Double (X), C_Double (Cycle)));
        end if;
    end Arccot;

end Ada.Numerics.Generic_Elementary_Functions;
