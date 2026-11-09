package body Ada.Strings.Maps is
    function To_Set (Ranges : Character_Ranges) return Character_Set is
        result : Character_Set := Null_Set;
    begin
        for i in Ranges'Range loop
            for c in Ranges (i).Low .. Ranges (i).High loop
                result.bits (c) := True;
            end loop;
        end loop;
        return result;
    end To_Set;

    function To_Set (Span : Character_Range) return Character_Set is
        result : Character_Set := Null_Set;
    begin
        for c in Span.Low .. Span.High loop
            result.bits (c) := True;
        end loop;
        return result;
    end To_Set;

    function To_Set (Sequence : Character_Sequence) return Character_Set is
        result : Character_Set := Null_Set;
    begin
        for i in Sequence'Range loop
            result.bits (Sequence (i)) := True;
        end loop;
        return result;
    end To_Set;

    function To_Set (Singleton : Character) return Character_Set is
        result : Character_Set := Null_Set;
    begin
        result.bits (Singleton) := True;
        return result;
    end To_Set;

    function To_Ranges (Set : Character_Set) return Character_Ranges is
        count : Natural := 0;
        inRange : Boolean := False;
    begin
        for c in Character loop
            if Set.bits (c) and not inRange then
                count := count + 1;
            end if;
            inRange := Set.bits (c);
        end loop;
        declare
            result : Character_Ranges (1 .. count);
            index : Natural := 0;
        begin
            inRange := False;
            for c in Character loop
                if Set.bits (c) then
                    if not inRange then
                        index := index + 1;
                        result (index).Low := c;
                    end if;
                    result (index).High := c;
                end if;
                inRange := Set.bits (c);
            end loop;
            return result;
        end;
    end To_Ranges;

    function To_Sequence (Set : Character_Set) return Character_Sequence is
        buffer : String (1 .. 256);
        count : Natural := 0;
    begin
        for c in Character loop
            if Set.bits (c) then
                count := count + 1;
                buffer (count) := c;
            end if;
        end loop;
        return buffer (1 .. count);
    end To_Sequence;

    function "=" (Left, Right : Character_Set) return Boolean is
    begin
        return Left.bits = Right.bits;
    end "=";

    function "not" (Right : Character_Set) return Character_Set is
        result : Character_Set;
    begin
        for c in Character loop
            result.bits (c) := not Right.bits (c);
        end loop;
        return result;
    end "not";

    function "and" (Left, Right : Character_Set) return Character_Set is
        result : Character_Set;
    begin
        for c in Character loop
            result.bits (c) := Left.bits (c) and Right.bits (c);
        end loop;
        return result;
    end "and";

    function "or" (Left, Right : Character_Set) return Character_Set is
        result : Character_Set;
    begin
        for c in Character loop
            result.bits (c) := Left.bits (c) or Right.bits (c);
        end loop;
        return result;
    end "or";

    function "xor" (Left, Right : Character_Set) return Character_Set is
        result : Character_Set;
    begin
        for c in Character loop
            result.bits (c) := Left.bits (c) xor Right.bits (c);
        end loop;
        return result;
    end "xor";

    function "-" (Left, Right : Character_Set) return Character_Set is
        result : Character_Set;
    begin
        for c in Character loop
            result.bits (c) := Left.bits (c) and not Right.bits (c);
        end loop;
        return result;
    end "-";

    function Is_In (Element : Character; Set : Character_Set) return Boolean is
    begin
        return Set.bits (Element);
    end Is_In;

    function Is_Subset (Elements : Character_Set; Set : Character_Set) return Boolean is
    begin
        for c in Character loop
            if Elements.bits (c) and not Set.bits (c) then
                return False;
            end if;
        end loop;
        return True;
    end Is_Subset;

    function "<=" (Left, Right : Character_Set) return Boolean is
    begin
        return Is_Subset (Left, Right);
    end "<=";

    function Value (Map : Character_Mapping; Element : Character) return Character is
    begin
        return Character'Val (Character'Pos (Element) + Map.offsets (Element));
    end Value;

    function To_Mapping (From, To : Character_Sequence) return Character_Mapping is
        result : Character_Mapping := Identity;
        seen : Character_Set := Null_Set;
    begin
        if From'Length /= To'Length then
            raise Translation_Error;
        end if;
        for i in From'Range loop
            if seen.bits (From (i)) then
                raise Translation_Error;
            end if;
            seen.bits (From (i)) := True;
            result.offsets (From (i)) := Character'Pos (To (To'First + (i - From'First)))
                - Character'Pos (From (i));
        end loop;
        return result;
    end To_Mapping;

    function To_Domain (Map : Character_Mapping) return Character_Sequence is
        buffer : String (1 .. 256);
        count : Natural := 0;
    begin
        for c in Character loop
            if Map.offsets (c) /= 0 then
                count := count + 1;
                buffer (count) := c;
            end if;
        end loop;
        return buffer (1 .. count);
    end To_Domain;

    function To_Range (Map : Character_Mapping) return Character_Sequence is
        buffer : String (1 .. 256);
        count : Natural := 0;
    begin
        for c in Character loop
            if Map.offsets (c) /= 0 then
                count := count + 1;
                buffer (count) := Value (Map, c);
            end if;
        end loop;
        return buffer (1 .. count);
    end To_Range;
end Ada.Strings.Maps;
