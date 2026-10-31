with Ada.Text_IO; use Ada.Text_IO;

procedure SubprogramCompletions is
    procedure Update (Value : in out Integer);
    function Increment (Value : Integer) return Integer;
    procedure Nothing is null;

    procedure Update (Value : in out Integer) is
    begin
        Value := Increment (Value);
    end Update;

    function Increment (Value : Integer) return Integer is
    begin
        return Value + 1;
    end Increment;

    Value : Integer := 41;
begin
    Nothing;
    Update (Value);
    Put_Line (Integer'Image (Value));
end SubprogramCompletions;
