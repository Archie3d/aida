with Ada.Text_IO;
with Pragma_Import_Provider; use Pragma_Import_Provider;
procedure PragmaImport is
    procedure Stop (Value : Integer);
    pragma Import (C, Stop, "__ada_test_no_return_returns");
    type Native_Callback is access procedure (Value : Integer);
    pragma Convention (C, Native_Callback);
    Action : Native_Callback := Stop'Access;
    pragma No_Return (Stop);
    procedure Alias (Value : Integer) renames Stop;
    Renamed_Action : Native_Callback := Alias'Access;
    procedure Ordinary (Value : Integer);
    pragma Import (C, Ordinary, "__ada_test_no_return_returns");
    procedure Guarded (Value : Integer) renames Ordinary;
    pragma No_Return (Guarded);
    generic
    function Get_Guarded return Native_Callback;
    function Get_Guarded return Native_Callback is
    begin
        return Guarded'Access;
    end Get_Guarded;
    function Instantiate is new Get_Guarded;
    Guarded_Action : Native_Callback := Instantiate;
    procedure Invoke (Action : Native_Callback; Value : Integer);
    pragma Import (C, Invoke, "__ada_test_no_return_invoke");

    Ada_Action : Callback := Get;
    Text : String (3 .. 6) := "fail";
    use type Callback;
begin
    if Action /= Renamed_Action or Ada_Action /= Foreign_Fail'Access then
        raise Constraint_Error;
    end if;
    Ordinary (42);
    begin
        Stop (42);
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        Action (42);
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        Invoke (Guarded_Action, 42);
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    begin
        Ada_Action (Text);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    begin
        Ada_Action ("okay");
        raise Constraint_Error;
    exception
        when Program_Error => null;
    end;
    Ada.Text_IO.Put_Line ("imported nonreturning procedure passed");
end PragmaImport;
