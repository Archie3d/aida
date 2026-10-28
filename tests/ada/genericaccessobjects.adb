with Ada.Text_IO;
procedure GenericAccessObjects is
    procedure Check is
        type Pointer is access all Integer;
        A : aliased Integer := 11;
        B : aliased Integer := 22;
        P : Pointer := A'Access;
        Q : Pointer := B'Access;
        Calls : Integer := 0;
        function Initial return Pointer is
        begin
            Calls := Calls + 1;
            return P;
        end Initial;
        generic
            type Ref is access all Integer;
            Value : Ref;
            Replacement : Ref;
            Alias_1 : in out Ref;
            Alias_2 : in out Ref;
        package Model is
            procedure Update;
            function Read return Integer;
        end Model;
        package body Model is
            procedure Update is
            begin
                Value.all := Value.all + 1;
                Alias_1 := Replacement;
                Alias_2.all := Alias_2.all + 2;
            end Update;
            function Read return Integer is
            begin
                return Value.all;
            end Read;
        end Model;
        package Instance is new Model (Pointer, Initial, Q, P, P);
        type Pointer_Array is array (1 .. 2) of Pointer;
        Cells : Pointer_Array := (A'Access, B'Access);
        function Index return Integer is
        begin
            Calls := Calls + 1;
            return 1;
        end Index;
        generic
            Value : in out Pointer;
            Defaulted : Pointer := Initial;
        procedure Replace;
        procedure Replace is
        begin
            Value := Defaulted;
        end Replace;
        procedure Set_Cell is new Replace (Cells (Index));
    begin
        if Calls /= 3 then raise Program_Error; end if;
        Instance.Update;
        if Instance.Read /= 12 or A /= 12 or B /= 24 or P /= Q then
            raise Program_Error;
        end if;
        Set_Cell;
        Set_Cell;
        if Calls /= 3 or Cells (1) /= A'Access then raise Program_Error; end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("GenericAccessObjects passed");
end GenericAccessObjects;
