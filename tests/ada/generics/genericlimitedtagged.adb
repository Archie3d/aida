with Ada.Text_IO;
procedure GenericLimitedTagged is
    procedure Check is
        package Types is
            type Link is access all Integer;
            type Root is tagged limited record
                Value : aliased Integer := 0;
                Self : Link;
            end record;
            function Read (Value : Root) return Integer;
            type Derived is new Root with record
                Extra : Integer := 100;
            end record;
            overriding function Read (Value : Derived) return Integer;
            function Make (N : Integer) return Derived;
            function Wide return Root'Class;
        end Types;
        package body Types is
            function Read (Value : Root) return Integer is
            begin
                return Value.Value;
            end Read;
            function Read (Value : Derived) return Integer is
            begin
                return Value.Value + Value.Extra;
            end Read;
            function Make (N : Integer) return Derived is
            begin
                return Result : Derived do
                    Result.Value := N;
                    Result.Self := Result.Value'Unchecked_Access;
                end return;
            end Make;
            function Wide return Root'Class is
            begin
                return Make (7);
            end Wide;
        end Types;
        use Types;
        generic
            Value : Root'Class;
        package Model is
            function Valid (N : Integer) return Boolean;
        end Model;
        package body Model is
            function Valid (N : Integer) return Boolean is
            begin
                return Value'Tag = Derived'Tag and Read (Value) = N
                    and Value.Self.all'Address = Value.Value'Address;
            end Valid;
        end Model;
        package From_Wide is new Model (Wide);
        package From_Specific is new Model (Make (9));
    begin
        if not From_Wide.Valid (107) or not From_Specific.Valid (109) then
            raise Program_Error;
        end if;
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
    Ada.Text_IO.Put_Line ("limited tagged objects passed");
end GenericLimitedTagged;
