with Ada.Text_IO;
with Generic_Family.Child.Nested;
with Generic_Family.Reset;
with Child_Parent.Alias;
with Other_Child_Parent.Cross;
with Child_Support;
procedure GenericChildren is
    procedure Check is
        package One is new Generic_Family (Integer, 1, Child_Support.Increment);
        package Two is new Generic_Family (Integer, 10, Child_Support.Increment);
        generic package Child_View renames One.Child;
        package A is new Child_View;
        package B is new One.Child (7);
        package C is new Two.Child;
        package Nested_A is new A.Nested;
        package Nested_C is new C.Nested;
        procedure Reset is new One.Reset;
        X : One.Cell;
        Y : Two.Cell;
    begin
        A.Update (X);
        B.Update (X);
        C.Update (Y);
        if One.Get (X) /= 3 or Two.Get (Y) /= 11
            or One.Count /= 10 or Two.Count /= 3
            or A.Calls /= 1 or B.Calls /= 1 or C.Calls /= 1
            or A.Parent_Seed /= 1 or C.Parent_Seed /= 10
            or Nested_A.Parent_Calls /= 1 or Nested_C.Parent_Calls /= 1
        then
            raise Program_Error;
        end if;
        Reset (X);
        if One.Get (X) /= 1 or One.Count /= 11 then raise Program_Error; end if;
        begin
            A.Fail;
            raise Program_Error;
        exception
            when One.Fault => null;
            when Two.Fault => raise Program_Error;
        end;
    end Check;
    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Good : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        function Count return Integer is
        begin
            return Good;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            for I in 1 .. 10 loop
                Check;
            end loop;
            Results.Passed;
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
    Library_Cell : Child_Parent.Cell;
begin
    Child_Parent.Alias.Update (Library_Cell);
    if Child_Parent.Get (Library_Cell) /= 11 or Child_Parent.Count /= 3 then
        raise Program_Error;
    end if;
    Child_Parent.Alias.Update (Library_Cell);
    Other_Child_Parent.Cross.Update (Library_Cell);
    if Child_Parent.Get (Library_Cell) /= 13 or Child_Parent.Count /= 9
        or Other_Child_Parent.Count /= 0
    then
        raise Program_Error;
    end if;
    Check;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("generic children passed");
end GenericChildren;
