with Ada.Text_IO; use Ada.Text_IO;
with Controlled_Result_Model; use Controlled_Result_Model;
with Controlled_Completion_Model;
with System; use type System.Address;
procedure ImportedControlled is
    package Completion renames Controlled_Completion_Model;

    function Make_Guard (Value : Integer := 42) return Guard;
    pragma Import (Ada, Make_Guard, "controlled_result_model__make");
    function Make_Limited return Completion.Limited_Guard;
    pragma Import (Ada, Make_Limited, "controlled_completion_model__make");
    function Make_Dynamic return Completion.Root'Class;
    pragma Import (Ada, Make_Dynamic, "controlled_completion_model__dynamic");

    function Make_Array return Guards;
    pragma Import (Ada, Make_Array, "controlled_completion_model__make_array");

    type Limited_Factory is access function return Completion.Limited_Guard;
    Limited_Callback : Limited_Factory := Make_Limited'Access;
    type Factory is access function (Value : Integer) return Guard;
    Callback : Factory := Make_Guard'Access;
    function Forward return Completion.Limited_Guard is
    begin
        return Limited_Callback.all;
    end Forward;
begin
    for Iteration in 1 .. 3 loop
        declare
            Direct : Guard := Make_Guard;
            Indirect : Guard := Callback (Iteration);
            Limited_Object : Completion.Limited_Guard := Forward;
            Dynamic : Completion.Root'Class := Make_Dynamic;
            Base : Completion.Root'Class := Completion.Root'(null record);
            From_Base : Guard := Completion.Select_Item (Base);
            From_Child : Guard := Completion.Select_Item (Dynamic);
            Items : Guards := Make_Array;
        begin
            Check (Direct.Data.Value = 42 and Indirect.Data.Value = Iteration);
            Check (Limited_Object.Original = Limited_Object'Address and Completion.Live = 1);
            Check (Dynamic in Completion.Child and Completion.Child (Dynamic).Item.Data.Value = 42);
            Check (From_Base.Data.Value = 11 and From_Child.Data.Value = 42);
            Check (Items'First = 3 and Items'Last = 4 and Items (4).Data.Value = 17);
            Check (Objects = 7 and Resources = 6);
        end;
        Check (Objects = 0 and Resources = 0 and Completion.Live = 0);
    end loop;

    Fail_Adjust := 1;
    begin
        declare
            Failed : Guard := Callback (1);
        begin
            raise Constraint_Error;
        end;
    exception
        when Program_Error => null;
    end;
    Check (Objects = 0 and Resources = 0);
    Put_Line ("imported controlled results passed");
end ImportedControlled;
