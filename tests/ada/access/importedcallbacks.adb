with Ada.Text_IO; use Ada.Text_IO;
with Callback_Import_Provider;
procedure ImportedCallbacks is
    function Absolute (Value : Integer) return Integer;
    pragma Import (C, Absolute, "abs");
    type C_Function is access function (Value : Integer) return Integer;
    pragma Convention (C, C_Function);
    C : C_Function := Absolute'Access;
    function Increment (Value : Integer) return Integer;
    pragma Import (Ada, Increment, "callback_import_provider__increment");
    type Ada_Function is access function (Value : Integer) return Integer;
    F : Ada_Function := Increment'Access;
    function Echo (Value : String) return String;
    pragma Import (Ada, Echo, "callback_import_provider__echo");
    type Text_Function is access function (Value : String) return String;
    Text : Text_Function := Echo'Access;
    function Alias (Value : Integer) return Integer renames Increment;
    G : Ada_Function := Alias'Access;
    function Fail (Value : Integer) return Integer;
    pragma Import (Ada, Fail, "callback_import_provider__fail");
    Bad : Ada_Function := Fail'Access;
begin
    if C (-42) /= 42 or F (41) /= 42 or G (41) /= 42 then raise Program_Error; end if;
    if Text ("callbacks") /= "callbacks" or Echo ("direct") /= "direct" then raise Program_Error; end if;
    if C /= Absolute'Access or F /= Increment'Access or F /= G then raise Program_Error; end if;
    begin
        declare X : Integer := Bad (0); begin raise Program_Error; end;
    exception
        when Constraint_Error => null;
    end;
    Put_Line ("imported callbacks passed");
end ImportedCallbacks;
