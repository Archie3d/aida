procedure GenericAttributeErrors is
    generic
        with function Unary (Object : Integer) return Integer;
    package Functions is
    end Functions;
    package Value_Not_Function is new Functions (Integer'First);
    package Pos_Profile is new Functions (Integer'Pos);
    package Val_Profile is new Functions (Integer'Val);
    package Called_Attribute is new Functions (Integer'Succ (1));
    Value : Integer := 1;
    package Object_Prefix is new Functions (Value'Succ);
    package Wrong_Result is new Functions (Integer'Image);
    generic
        with function Binary (Left, Right : Integer) return Integer;
    package Binary_Functions is
    end Binary_Functions;
    package Wrong_Arity is new Binary_Functions (Integer'Succ);
    generic
        with procedure Action (Object : out Integer);
    package Procedures is
    end Procedures;
    package Wrong_Kind is new Procedures (Integer'Succ);
    generic
        with function Invalid (Object : Integer) return Integer is Integer'Image;
    package Invalid_Default is
    end Invalid_Default;
    generic
        type T is private;
        with function Encode (Object : T) return String is T'Image;
    package Private_Default is
    end Private_Default;
    type Unsigned is mod 256;
    package Mod_Profile is new Functions (Unsigned'Mod);
    type Decimal is delta 0.01 digits 6;
    package Round_Profile is new Functions (Decimal'Round);
    package Stream_Profile is new Functions (Integer'Input);
begin
    null;
end GenericAttributeErrors;
