with Ada.Text_IO;
with Rejection_Model;
procedure SupportedRejections is
    use type Rejection_Model.Tagged_Item'Class, Integer'Base;
    procedure Run is
        type Vector is array (Integer range 1 .. 5) of Integer;
        type Flags is array (Boolean) of Boolean;
        type Pointer is access all Vector;
        type Other_Pointer is access Vector;
        Data : aliased Vector := (1, 2, 3, 4, 5);
        Calls, Index_Calls : Natural := 0;
        Return_Null : Boolean := False;
        function Fetch return Pointer is
        begin
            Calls := Calls + 1;
            if Return_Null then
                return null;
            end if;
            return Data'Access;
        end Fetch;
        function Fetch return Flags is
        begin
            return (False, True);
        end Fetch;
        function Copied return Vector is
        begin
            Calls := Calls + 1;
            return Data;
        end Copied;
        function Copied (Index : Integer) return Boolean is
        begin
            return False;
        end Copied;
        function Position return Integer is
        begin
            Index_Calls := Index_Calls + 1;
            return 3;
        end Position;
        procedure Change (Value : in out Integer) is
        begin
            Value := Value + 10;
        end Change;
        function Defaulted (Unused : Boolean := False) return Vector is
        begin
            return Data;
        end Defaulted;
        Selected : Integer := 0;
        Specific : Rejection_Model.Tagged_Item := (Value => 1);
        package Outer_Package is
            Value : Integer := 7;
            package Inner_Package is
                Count : Integer := 11;
                function Result return Integer;
            end Inner_Package;
        end Outer_Package;
        package body Outer_Package is
            Body_Local : Integer := 7;
            function Add (Value : Integer) return Integer is
            begin
                return Value + Body_Local;
            end Add;
            package body Inner_Package is
                Local : Integer := Add (Count);
                function Result return Integer is
                begin
                    return Body_Local + Local;
                end Result;
            begin
                Body_Local := Body_Local * 2;
                Value := Value * 5;
                Count := Count * 2;
                if Body_Local /= 14 or Local /= 18 then
                    raise Program_Error;
                end if;
            end Inner_Package;
        begin
            Value := Value + 20;
            Body_Local := Body_Local + 20;
        end Outer_Package;
        Item : Rejection_Model.Item (3) := Rejection_Model.Create (3);
        Left, Right : Rejection_Model.Tagged_Item'Class := Rejection_Model.Tagged_Item'(Value => 1);
    begin
        if Copied (Position) /= 3 or Copied (1) or Fetch (False) or not Fetch (True) then
            raise Program_Error;
        end if;
        Change (Fetch (3));
        if Calls /= 2 or Index_Calls /= 1 or Data (3) /= 13 then
            raise Program_Error;
        end if;
        Fetch (4) := 14;
        if Data (4) /= 14 or Calls /= 3 or Defaulted (4) /= 14 then
            raise Program_Error;
        end if;
        if Data (2 .. 4)(3) /= 13 or Copied (2 .. 4)(4) /= 14 then
            raise Program_Error;
        end if;
        Data (2 .. 4)(2) := 12;
        Change (Data (2 .. 4)(4));
        if Data /= (1, 12, 13, 24, 5) then
            raise Program_Error;
        end if;
        Specific := Specific + 2;
        if Specific.Value /= 3 or Rejection_Model.Values (6) /= 9 or not Rejection_Model.Check
            or Item.Length /= 3 or Left /= Right
            or Outer_Package.Value /= 55 or Outer_Package.Inner_Package.Count /= 22
            or Outer_Package.Inner_Package.Result /= 52 then
            raise Program_Error;
        end if;
        Return_Null := True;
        begin
            Selected := Fetch (1);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Selected := Copied (2 .. 4)(1);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        if Selected /= 0 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("supported-area worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("supported-area rejections repaired");
end SupportedRejections;
