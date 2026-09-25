with Ada.Text_IO;
procedure Generic_Composite_Checks is
    type Vector is array (Integer range <>) of Integer;
    subtype Triple is Vector (1 .. 3);
    generic
        Value : Triple;
    package Checked is
    end Checked;
    generic
        Target : in out Vector;
        Source : Vector;
    procedure Replace;
    procedure Replace is
    begin
        Target := Source;
    end Replace;
    Size : Integer := 2;
    Short : Vector (5 .. Size + 4) := (others => 7);
    Target : Triple := (1, 2, 3);
    Static_Short : Vector (1 .. 2) := (7, 8);
    procedure Bad_Copy is new Replace (Target, Short);
    type Item (Kind : Boolean) is record
        Value : Integer;
    end record;
    subtype True_Item is Item (True);
    A : True_Item := (True, 12);
    B : Item (False) := (False, 20);
    generic
        Value : True_Item;
    package Record_Check is
    end Record_Check;
    generic
        Target : in out Item;
        Source : Item;
    procedure Assign_Record;
    procedure Assign_Record is
    begin
        Target := Source;
    end Assign_Record;
    procedure Bad_Record is new Assign_Record (A, B);
    Caught : Integer := 0;
begin
    begin
        declare
            package Wrong_Length is new Checked (Short);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        declare
            package Static_Length is new Checked (Static_Short);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        Bad_Copy;
        raise Program_Error;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        declare
            package Wrong_Discriminant is new Record_Check (B);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    begin
        Bad_Record;
        raise Program_Error;
    exception
        when Constraint_Error => Caught := Caught + 1;
    end;
    if Caught /= 5 or Target /= (1, 2, 3) or A.Kind /= True or A.Value /= 12 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("generic composite shape and discriminant checks preserve targets");
end Generic_Composite_Checks;
