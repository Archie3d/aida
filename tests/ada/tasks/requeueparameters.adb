with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure RequeueParameters is
    type Vector is array (Integer range <>) of Integer;
    type Root is tagged record Value : Integer := 0; end record;
    type Child is new Root with record Extra : Integer := 4; end record;
    Finalized : Natural := 0;
    pragma Atomic (Finalized);
    type Item is new Ada.Finalization.Limited_Controlled with record Value : Integer := 0; end record;
    overriding procedure Finalize (Object : in out Item) is
    begin Finalized := Finalized + 1; end Finalize;
    type Callback is access procedure (N : in out Integer);
    procedure Add_Five (N : in out Integer) is begin N := N + 5; end Add_Five;
    protected Gate is
        entry Finish (Data : in out Vector; Label : String; Result : out Integer;
            Controlled : in out Item; View : in out Root'Class; Action : Callback);
    end Gate;
    protected body Gate is
        entry Finish (Data : in out Vector; Label : String; Result : out Integer;
            Controlled : in out Item; View : in out Root'Class; Action : Callback) when True is
        begin
            if Data'First /= 4 or Data'Last /= 6 or Label /= "relay" or Finalized /= 0 then raise Program_Error; end if;
            Data (Data'First) := Data (Data'First) + 3;
            Controlled.Value := Controlled.Value + 3;
            View.Value := View.Value + 3;
            Result := Result + 3;
            Action (Result);
        end Finish;
    end Gate;
    task Middle is
        entry Work (Data : in out Vector; Label : String; Result : out Integer;
            Controlled : in out Item; View : in out Root'Class; Action : Callback);
    end Middle;
    task body Middle is
    begin
        accept Work (Data : in out Vector; Label : String; Result : out Integer;
            Controlled : in out Item; View : in out Root'Class; Action : Callback) do
            if Data'First /= 4 or Data'Last /= 6 or Label /= "relay" or Finalized /= 0 then raise Program_Error; end if;
            Data (Data'First) := Data (Data'First) + 2;
            Controlled.Value := Controlled.Value + 2;
            View.Value := View.Value + 2;
            Result := Result + 2;
            requeue Gate.Finish;
        end Work;
    end Middle;
    task First is
        entry Work (Data : in out Vector; Label : String; Result : out Integer;
            Controlled : in out Item; View : in out Root'Class; Action : Callback);
    end First;
    task body First is
    begin
        accept Work (Data : in out Vector; Label : String; Result : out Integer;
            Controlled : in out Item; View : in out Root'Class; Action : Callback) do
            Data (Data'First) := Data (Data'First) + 1;
            Controlled.Value := Controlled.Value + 1;
            View.Value := View.Value + 1;
            Result := 1;
            requeue Middle.Work;
        end Work;
    end First;
begin
    declare
        Data : Vector (3 .. 7) := (1, 2, 3, 4, 5);
        Controlled : Item;
        View : Child;
        Result : Integer := 99;
    begin
        First.Work (Data (4 .. 6), "relay", Result, Controlled, View, Add_Five'Access);
        if Data (3) /= 1 or Data (4) /= 8 or Data (7) /= 5 or Result /= 11
            or Controlled.Value /= 6 or View.Value /= 6 or View.Extra /= 4 or Finalized /= 0 then
            raise Program_Error;
        end if;
    end;
    if Finalized /= 1 then raise Program_Error; end if;
    Put_Line ("requeue parameters OK");
end RequeueParameters;
