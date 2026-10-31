with Ada.Text_IO; use Ada.Text_IO;
procedure Checkedarithmetic is
    function Reference (Operation, Bits : Integer; A, B : Long_Integer;
                        Result : out Long_Integer) return Integer;
    pragma Import (C, Reference, "checkedArithmetic");
    function Sample (Index, Bits : Integer) return Long_Integer;
    pragma Import (C, Sample, "arithmeticSample");

    function Evaluate32 (Operation : Integer; A, B : Integer) return Integer is
    begin
        case Operation is
            when 0 => return A + B;
            when 1 => return A - B;
            when 2 => return A * B;
            when 3 => return A / B;
            when 4 => return A rem B;
            when 5 => return A mod B;
            when 6 => return A ** B;
            when 7 => return -A;
            when others => return abs A;
        end case;
    end Evaluate32;

    function Evaluate64 (Operation : Integer; A, B : Long_Integer) return Long_Integer is
    begin
        case Operation is
            when 0 => return A + B;
            when 1 => return A - B;
            when 2 => return A * B;
            when 3 => return A / B;
            when 4 => return A rem B;
            when 5 => return A mod B;
            when 6 => return A ** Integer (B);
            when 7 => return -A;
            when others => return abs A;
        end case;
    end Evaluate64;

    procedure Check (Operation, Bits : Integer; A, B : Long_Integer) is
        Expected : Long_Integer := 0;
        Actual : Long_Integer := 123;
        Valid : Integer := Reference (Operation, Bits, A, B, Expected);
        Caught : Boolean := False;
    begin
        begin
            if Bits = 32 then
                Actual := Long_Integer (Evaluate32 (Operation, Integer (A), Integer (B)));
            else
                Actual := Evaluate64 (Operation, A, B);
            end if;
        exception
            when Constraint_Error => Caught := True;
        end;
        if (Valid = 0 and then (not Caught or else Actual /= 123))
          or else (Valid /= 0 and then (Caught or else Actual /= Expected)) then
            Put_Line ("mismatch" & Integer'Image (Operation) & Integer'Image (Bits)
                      & Long_Integer'Image (A) & Long_Integer'Image (B));
            raise Program_Error;
        end if;
    end Check;
    Bits : Integer;
    A, B : Long_Integer;
    Calls : Integer := 0;
    function Once return Integer is
    begin
        Calls := Calls + 1;
        return 3;
    end Once;
    Value : Integer;
begin
    for Width in 1 .. 2 loop
        Bits := Width * 32;
        for I in 0 .. 79 loop
            A := Sample (I, Bits);
            for J in 0 .. 79 loop
                B := Sample (J, Bits);
                for Operation in 0 .. 5 loop
                    Check (Operation, Bits, A, B);
                end loop;
            end loop;
            for Exponent in -1 .. 65 loop
                Check (6, Bits, A, Long_Integer (Exponent));
            end loop;
            Check (6, Bits, A, 2_147_483_647);
            Check (7, Bits, A, 0);
            Check (8, Bits, A, 0);
        end loop;
    end loop;
    Value := Once ** Once;
    if Calls /= 2 or else Value /= 27 then
        raise Program_Error;
    end if;
    Put_Line ("checked arithmetic passed");
end Checkedarithmetic;
