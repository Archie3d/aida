package body Ada.Strings.Wide_Maps is
    function To_Set (Ranges : Wide_Character_Ranges) return Wide_Character_Set is
        Result : Wide_Character_Set;
    begin
        for I in Ranges'Range loop
            Result.value := Wide_Support.Combine (Result.value,
                Wide_Support.Singleton (Wide_Character'Pos (Ranges (I).Low),
                    Wide_Character'Pos (Ranges (I).High)), Wide_Support.Union);
        end loop;
        return Result;
    end To_Set;
    function To_Set (Span : Wide_Character_Range) return Wide_Character_Set is
    begin
        return (value => Wide_Support.Singleton (Wide_Character'Pos (Span.Low), Wide_Character'Pos (Span.High)));
    end To_Set;
    function To_Set (Sequence : Wide_Character_Sequence) return Wide_Character_Set is
        Result : Wide_Character_Set;
    begin
        for I in Sequence'Range loop
            Result := Result or To_Set (Sequence (I));
        end loop;
        return Result;
    end To_Set;
    function To_Set (Singleton : Wide_Character) return Wide_Character_Set is
    begin
        return To_Set (Wide_Character_Range'(Singleton, Singleton));
    end To_Set;
    function To_Ranges (Set : Wide_Character_Set) return Wide_Character_Ranges is
        Spans : Wide_Support.Spans := Wide_Support.Ranges (Set.value);
        Result : Wide_Character_Ranges (1 .. Spans'Length);
    begin
        for I in Result'Range loop
            Result (I) := (Wide_Character'Val (Spans (I).Low), Wide_Character'Val (Spans (I).High));
        end loop;
        return Result;
    end To_Ranges;
    function To_Sequence (Set : Wide_Character_Set) return Wide_Character_Sequence is
        Spans : Wide_Character_Ranges := To_Ranges (Set);
        Length : Long_Integer := 0;
    begin
        for I in Spans'Range loop
            Length := Length + Wide_Character'Pos (Spans (I).High) - Wide_Character'Pos (Spans (I).Low) + 1;
        end loop;
        if Length > Long_Integer (Natural'Last) then
            raise Storage_Error;
        end if;
        declare
            Result : Wide_String (1 .. Natural (Length));
            Position : Natural := 0;
        begin
            for I in Spans'Range loop
                for C in Spans (I).Low .. Spans (I).High loop
                    Position := Position + 1;
                    Result (Position) := C;
                end loop;
            end loop;
            return Result;
        end;
    end To_Sequence;
    function "=" (Left, Right : Wide_Character_Set) return Boolean is
    begin
        return To_Ranges (Left) = To_Ranges (Right);
    end "=";
    function "not" (Right : Wide_Character_Set) return Wide_Character_Set is
    begin
        return (value => Wide_Support.Combine (Wide_Support.Singleton (0, Wide_Character'Pos (Wide_Character'Last)), Right.value, Wide_Support.Difference));
    end "not";
    function "and" (Left, Right : Wide_Character_Set) return Wide_Character_Set is
    begin
        return (value => Wide_Support.Combine (Left.value, Right.value, Wide_Support.Intersection));
    end "and";
    function "or" (Left, Right : Wide_Character_Set) return Wide_Character_Set is
    begin
        return (value => Wide_Support.Combine (Left.value, Right.value, Wide_Support.Union));
    end "or";
    function "xor" (Left, Right : Wide_Character_Set) return Wide_Character_Set is
    begin
        return (value => Wide_Support.Combine (Left.value, Right.value, Wide_Support.Exclusive_Or));
    end "xor";
    function "-" (Left, Right : Wide_Character_Set) return Wide_Character_Set is
    begin
        return (value => Wide_Support.Combine (Left.value, Right.value, Wide_Support.Difference));
    end "-";
    function Is_In (Element : Wide_Character; Set : Wide_Character_Set) return Boolean is
    begin
        return Wide_Support.Contains (Set.value, Wide_Character'Pos (Element));
    end Is_In;
    function Is_Subset (Elements : Wide_Character_Set; Set : Wide_Character_Set) return Boolean is
    begin
        return Elements - Set = Null_Set;
    end Is_Subset;
    function "<=" (Left, Right : Wide_Character_Set) return Boolean is
    begin
        return Is_Subset (Left, Right);
    end "<=";
    function Value (Map : Wide_Character_Mapping; Element : Wide_Character) return Wide_Character is
    begin
        return Wide_Character'Val (Wide_Support.Value (Map.value, Wide_Character'Pos (Element)));
    end Value;
    function To_Mapping (From, To : Wide_Character_Sequence) return Wide_Character_Mapping is
        Result : Wide_Character_Mapping;
    begin
        if From'Length /= To'Length then
            raise Translation_Error;
        end if;
        for I in 0 .. From'Length - 1 loop
            for J in 0 .. I - 1 loop
                if From (From'First + I) = From (From'First + J) then
                    raise Translation_Error;
                end if;
            end loop;
            Wide_Support.Add (Result.value, Wide_Character'Pos (From (From'First + I)),
                Wide_Character'Pos (To (To'First + I)));
        end loop;
        return Result;
    end To_Mapping;
    function To_Domain (Map : Wide_Character_Mapping) return Wide_Character_Sequence is
        Spans : Wide_Support.Spans := Wide_Support.Domain (Map.value);
        Result : Wide_Character_Set;
    begin
        for I in Spans'Range loop
            Result.value := Wide_Support.Combine (Result.value,
                Wide_Support.Singleton (Spans (I).Low, Spans (I).High), Wide_Support.Union);
        end loop;
        return To_Sequence (Result);
    end To_Domain;
    function To_Range (Map : Wide_Character_Mapping) return Wide_Character_Sequence is
        Result : Wide_String := To_Domain (Map);
    begin
        for I in Result'Range loop
            Result (I) := Value (Map, Result (I));
        end loop;
        return Result;
    end To_Range;
end Ada.Strings.Wide_Maps;
