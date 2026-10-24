with Ada.Text_IO; use Ada.Text_IO;
procedure ImportedCallbackABI is
    type C_Function is access function (Value : Integer) return Integer;
    pragma Convention (C, C_Function);
    function Absolute (Value : Integer) return Integer;
    pragma Import (C, Absolute, "abs");
    function Apply (Action : C_Function; Value : Integer) return Integer;
    pragma Import (C, Apply, "__test_callback_call");
    function Get return C_Function;
    pragma Import (C, Get, "__test_callback_get");
    type C_Factory is access function return C_Function;
    pragma Convention (C, C_Factory);
    Factory : C_Factory := Get'Access;
    F : C_Function := Factory.all;
    function Single (Value : Float) return Float;
    pragma Import (C, Single, "__test_callback_float");
    function Double (Value : Long_Float) return Long_Float;
    pragma Import (C, Double, "__test_callback_double");
    procedure Update (Value : in out Integer);
    pragma Import (C, Update, "__test_callback_update");
    type C_Single is access function (Value : Float) return Float;
    pragma Convention (C, C_Single);
    type C_Double is access function (Value : Long_Float) return Long_Float;
    pragma Convention (C, C_Double);
    type C_Update is access procedure (Value : in out Integer);
    pragma Convention (C, C_Update);
    S : C_Single := Single'Access;
    D : C_Double := Double'Access;
    P : C_Update := Update'Access;
    Value : Integer := 35;
begin
    if F /= Absolute'Access or Apply (F, -42) /= 42 or F (-42) /= 42 then raise Program_Error; end if;
    if S (1.0) /= 1.5 or D (1.0) /= 1.25 then raise Program_Error; end if;
    P (Value);
    if Value /= 42 then raise Program_Error; end if;
    F := null;
    begin
        Value := F (1);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Put_Line ("C callback ABI passed");
end ImportedCallbackABI;
