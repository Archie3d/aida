package body Real_Value_Checks is
    procedure Run is
        subtype Small is Real range -1.0 .. 1.0;
        Value : Real := 0.0;
        Calls : Integer := 0;
        Text : String (11 .. 18) := " 2#1.1# ";
        function Once return String is
        begin
            Calls := Calls + 1;
            return Text (12 .. 17);
        end Once;
        procedure Invalid (Text : String) is
        begin
            Value := Real'Value (Text);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end Invalid;
    begin
        if Real'Value (" +1_2.5_0e-1 ") /= 1.25
            or Real'Value ("16#f.f#e+1") /= 255.0
            or Real'Value ("3#1.2#E1") /= 5.0
            or Real'Value ("2#.1#") /= 0.5
            or Real'Value ("16#A.#") /= 10.0
            or Real'Value (".125") /= 0.125
            or Real'Value ("-1.") /= -1.0
            or Real'Value ("2e-1") /= 0.2
            or Real'Value ("1_6:a.8:") /= 10.5
        then
            raise Program_Error;
        end if;
        Value := Real'Value (Once);
        if Value /= 1.5 or Calls /= 1 then
            raise Program_Error;
        end if;
        -- The attribute returns Small'Base. Assignment checks Small separately.
        Value := Small'Value ("2.0");
        if Value /= 2.0 then
            raise Program_Error;
        end if;
        declare
            Narrow : Small;
        begin
            Narrow := Small'Value ("2.0");
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        if Real'Value ("0e999999999999999999999") /= 0.0
            or Real'Value ("1e-999999999999999999999") /= 0.0
        then
            raise Program_Error;
        end if;
        Invalid ("");
        Invalid ("   ");
        Invalid (".");
        Invalid ("+ 1");
        Invalid ("1__0");
        Invalid ("1_");
        Invalid ("1._0");
        Invalid ("1e+");
        Invalid ("1e1_");
        Invalid ("1e999999999999999999999x");
        Invalid ("1e999999999999999999999");
        Invalid ("1e-999999999999999999999_");
        Invalid ("nan");
        Invalid ("inf");
        Invalid ("0x1p0");
        Invalid ("2#2#");
        Invalid ("1#0#");
        Invalid ("17#0#");
        Invalid ("16#1:");
        Invalid ("2##");
        Invalid ("1.0 2");
        Invalid ("1.0" & Character'Val (0));
        Invalid (Character'Val (9) & "1.0");
    end Run;
end Real_Value_Checks;
