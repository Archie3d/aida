with Ada.Text_IO;

procedure Pragmas is
    package Pure_Values is
        Value : constant Integer := 17;
    end Pure_Values;

    package Preelaborated_Values is
        Value : constant Integer := 25;
    end Preelaborated_Values;

    package More_Values is
        Value : constant Integer := 0;
    end More_Values;

    package More_Preelaborated_Values is
        Value : constant Integer := 0;
    end More_Preelaborated_Values;

    function First return Integer is
    begin
        return Pure_Values.Value;
    end First;

    function Second return Integer is
    begin
        return Preelaborated_Values.Value;
    end Second;

    pragma iNlInE (First, Second);
    pragma Inline (Entity => First);

    procedure Print (Value : String);
    pragma Import (C, Print, "__ada_put_line");
begin
    Print (Integer'Image (First + Second));
end Pragmas;
