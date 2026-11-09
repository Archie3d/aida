with Ada.Strings.Maps.Constants;
package body Ada.Strings.Wide_Support is
    -- Fixed-width, big-endian encoding is private storage, not a stream format.
    function Encode (Point : Long_Integer) return String is
        Result : String (1 .. 4);
        Rest : Long_Integer := Point;
    begin
        for I in reverse Result'Range loop
            Result (I) := Character'Val (Rest mod 256);
            Rest := Rest / 256;
        end loop;
        return Result;
    end Encode;

    function Decode (Data : Unbounded.Unbounded_String; Offset : Natural)
        return Long_Integer is
        Result : Long_Integer := 0;
    begin
        for I in 1 .. 4 loop
            Result := Result * 256 + Character'Pos (Unbounded.Element (Data, Offset + I));
        end loop;
        return Result;
    end Decode;

    function Contains (Object : Set; Point : Long_Integer) return Boolean is
    begin
        if Point < Object.Prefix then
            return True;
        end if;
        if Point <= 255 then
            case Object.Base is
                when Control =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Control_Set) then
                        return True;
                    end if;
                when Graphic =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Graphic_Set) then
                        return True;
                    end if;
                when Letter =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Letter_Set) then
                        return True;
                    end if;
                when Lower =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Lower_Set) then
                        return True;
                    end if;
                when Upper =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Upper_Set) then
                        return True;
                    end if;
                when Basic =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Basic_Set) then
                        return True;
                    end if;
                when Decimal_Digit =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Decimal_Digit_Set) then
                        return True;
                    end if;
                when Hexadecimal_Digit =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Hexadecimal_Digit_Set) then
                        return True;
                    end if;
                when Alphanumeric =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Alphanumeric_Set) then
                        return True;
                    end if;
                when Special =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.Special_Set) then
                        return True;
                    end if;
                when ISO_646 =>
                    if Maps.Is_In (Character'Val (Point), Maps.Constants.ISO_646_Set) then
                        return True;
                    end if;
                when Empty => null;
            end case;
        end if;
        for I in 0 .. Unbounded.Length (Object.Data) / 8 - 1 loop
            if Point >= Decode (Object.Data, I * 8) and
                Point <= Decode (Object.Data, I * 8 + 4) then
                return True;
            end if;
        end loop;
        return False;
    end Contains;

    function Singleton (Low, High : Long_Integer) return Set is
        Result : Set;
    begin
        if Low <= High then
            Result.Data := Unbounded.To_Unbounded_String (Encode (Low) & Encode (High));
        end if;
        return Result;
    end Singleton;

    function Combine (Left, Right : Set; Operation : Set_Operation) return Set is
        type Points is array (Positive range <>) of Long_Integer;
        Edges : Points (1 .. 262 + Unbounded.Length (Left.Data) / 4 +
            Unbounded.Length (Right.Data) / 4);
        Count : Natural := 0;
        Result : Set;
        Active : Boolean := False;
        Start : Long_Integer := 0;
        Selected, L, R : Boolean;
        procedure Edge (Point : Long_Integer) is
            Position : Positive := Count + 1;
        begin
            while Position > 1 loop
                exit when Edges (Position - 1) <= Point;
                Edges (Position) := Edges (Position - 1);
                Position := Position - 1;
            end loop;
            Edges (Position) := Point;
            Count := Count + 1;
        end Edge;
        procedure Gather (Object : Set) is
        begin
            Edge (0);
            Edge (Object.Prefix);
            for I in 0 .. Unbounded.Length (Object.Data) / 8 - 1 loop
                Edge (Decode (Object.Data, I * 8));
                Edge (Decode (Object.Data, I * 8 + 4) + 1);
            end loop;
        end Gather;
    begin
        for I in 0 .. 256 loop
            Edge (Long_Integer (I));
        end loop;
        Edge (2 ** 31);
        Gather (Left);
        Gather (Right);
        for I in 1 .. Count - 1 loop
            if Edges (I) < Edges (I + 1) then
                L := Contains (Left, Edges (I));
                R := Contains (Right, Edges (I));
                if Operation = Union then
                    Selected := L or R;
                elsif Operation = Intersection then
                    Selected := L and R;
                elsif Operation = Exclusive_Or then
                    Selected := L xor R;
                else
                    Selected := L and not R;
                end if;
                if Selected and not Active then
                    Start := Edges (I);
                elsif Active and not Selected then
                    Unbounded.Append (Result.Data, Encode (Start) & Encode (Edges (I) - 1));
                end if;
                Active := Selected;
            end if;
        end loop;
        if Active then
            Unbounded.Append (Result.Data, Encode (Start) & Encode (Edges (Count) - 1));
        end if;
        return Result;
    end Combine;

    function Ranges (Object : Set) return Spans is
        Canonical : Set := Combine (Object, Null_Set, Union);
        Result : Spans (1 .. Unbounded.Length (Canonical.Data) / 8);
    begin
        for I in Result'Range loop
            Result (I).Low := Decode (Canonical.Data, (I - 1) * 8);
            Result (I).High := Decode (Canonical.Data, (I - 1) * 8 + 4);
        end loop;
        return Result;
    end Ranges;

    function Value (Object : Mapping; Point : Long_Integer) return Long_Integer is
    begin
        for I in 0 .. Unbounded.Length (Object.Data) / 8 - 1 loop
            if Decode (Object.Data, I * 8) = Point then
                return Decode (Object.Data, I * 8 + 4);
            end if;
        end loop;
        if Point <= 255 then
            case Object.Base is
                when Lower_Case => return Character'Pos (Maps.Value (Maps.Constants.Lower_Case_Map, Character'Val (Point)));
                when Upper_Case => return Character'Pos (Maps.Value (Maps.Constants.Upper_Case_Map, Character'Val (Point)));
                when Basic_Case => return Character'Pos (Maps.Value (Maps.Constants.Basic_Map, Character'Val (Point)));
                when Unmapped => null;
            end case;
        end if;
        return Point;
    end Value;

    procedure Add (Object : in out Mapping; From, Target : Long_Integer) is
        Position : Natural := 0;
    begin
        if From = Target then
            return;
        end if;
        while Position < Unbounded.Length (Object.Data) loop
            exit when Decode (Object.Data, Position) > From;
            Position := Position + 8;
        end loop;
        Unbounded.Insert (Object.Data, Position + 1, Encode (From) & Encode (Target));
    end Add;

    function Domain (Object : Mapping) return Spans is
        Result : Set;
    begin
        for I in 0 .. 255 loop
            if Value (Object, Long_Integer (I)) /= Long_Integer (I) then
                Result := Combine (Result, Singleton (Long_Integer (I), Long_Integer (I)), Union);
            end if;
        end loop;
        for I in 0 .. Unbounded.Length (Object.Data) / 8 - 1 loop
            declare
                Point : Long_Integer := Decode (Object.Data, I * 8);
            begin
                Result := Combine (Result, Singleton (Point, Point), Union);
            end;
        end loop;
        return Ranges (Result);
    end Domain;
end Ada.Strings.Wide_Support;
