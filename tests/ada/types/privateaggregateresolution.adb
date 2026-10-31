with Ada.Text_IO;

procedure PrivateAggregateResolution is
    type Item is record
        Value : Integer;
    end record;
    package Hidden is
        type Record_View is private;
        type Array_View is private;
        function Pick (Value : Item) return Integer;
        function Pick (Value : Record_View) return Integer;
        function Pick (Value : Array_View) return Integer;
        function Inside return Integer;
    private
        type Record_View is record
            Value : Integer;
        end record;
        type Array_View is array (1 .. 1) of Integer;
    end Hidden;
    package body Hidden is
        function Pick (Value : Item) return Integer is
        begin
            return Value.Value;
        end Pick;
        function Pick (Value : Record_View) return Integer is
        begin
            return Value.Value + 10;
        end Pick;
        function Pick (Value : Array_View) return Integer is
        begin
            return Value (1) + 20;
        end Pick;
        function Read_Record (Value : Record_View) return Integer is
        begin
            return Value.Value;
        end Read_Record;
        function Read_Array (Value : Array_View) return Integer is
        begin
            return Value (1);
        end Read_Array;
        function Inside return Integer is
        begin
            -- Full views remain usable inside their package body.
            return Read_Record ((Value => 4)) + Read_Array ((1 => 5));
        end Inside;
    end Hidden;
begin
    if Hidden.Pick ((Value => 7)) /= 7 or else Hidden.Inside /= 9 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("private aggregate resolution ok");
end PrivateAggregateResolution;
