with Ada.Strings.Maps;
package body Ada.Strings.Fixed is
    -- Work with lengths and offsets, so valid strings ending at Integer'Last
    -- never require an unrepresentable one-past-the-end index.
    function segment (source : String; offset, length : Natural) return String is
        result : String (1 .. length);
    begin
        for i in result'Range loop
            result (i) := source (source'First + (offset + (i - 1)));
        end loop;
        return result;
    end segment;

    function matches (source, pattern : String; offset : Natural;
                      mapping : Maps.Character_Mapping) return Boolean is
    begin
        for j in 0 .. pattern'Length - 1 loop
            if Maps.Value (mapping, source (source'First + (offset + j))) /=
                pattern (pattern'First + j) then
                return False;
            end if;
        end loop;
        return True;
    end matches;

    procedure Move (Source : String; Target : out String; Drop : Truncation := Error;
                   Justify : Alignment := Left; Pad : Character := Space) is
        result : String (1 .. Target'Length) := (others => Pad);
        sourceOffset : Natural := 0;
        targetOffset : Natural := 0;
        copyLength : Natural := Source'Length;
        excess : Natural;
    begin
        if Source'Length > Target'Length then
            excess := Source'Length - Target'Length;
            copyLength := Target'Length;
            if Drop = Left then
                sourceOffset := excess;
            elsif Drop = Error then
                if Justify = Center then
                    raise Length_Error;
                elsif Justify = Right then
                    sourceOffset := excess;
                end if;
                for i in 0 .. excess - 1 loop
                    if Justify = Left then
                        if Source (Source'First + (copyLength + i)) /= Pad then
                            raise Length_Error;
                        end if;
                    elsif Source (Source'First + i) /= Pad then
                        raise Length_Error;
                    end if;
                end loop;
            end if;
        elsif Justify = Right then
            targetOffset := Target'Length - Source'Length;
        elsif Justify = Center then
            targetOffset := (Target'Length - Source'Length) / 2;
        end if;
        for i in 0 .. copyLength - 1 loop
            result (1 + (targetOffset + i)) := Source (Source'First + (sourceOffset + i));
        end loop;
        -- Commit only after validation and copying; Source may overlap Target.
        Target := result;
    end Move;

    function Index (Source, Pattern : String; From : Positive;
                    Going : Direction := Forward; Mapping : Maps.Character_Mapping := Maps.Identity) return Natural is
        lowOffset : Natural := 0;
        highOffset : Integer;
    begin
        if Pattern'Length = 0 then
            raise Pattern_Error;
        end if;
        if Source'Length = 0 then
            return 0;
        end if;
        if From < Source'First or From > Source'Last then
            raise Index_Error;
        end if;
        if Going = Forward then
            lowOffset := From - Source'First;
            highOffset := Source'Length - Pattern'Length;
            for i in lowOffset .. highOffset loop
                if matches (Source, Pattern, i, Mapping) then
                    return Source'First + i;
                end if;
            end loop;
        else
            highOffset := (From - Source'First) - (Pattern'Length - 1);
            for i in reverse 0 .. highOffset loop
                if matches (Source, Pattern, i, Mapping) then
                    return Source'First + i;
                end if;
            end loop;
        end if;
        return 0;
    end Index;

    function Index (Source, Pattern : String; Going : Direction := Forward;
                    Mapping : Maps.Character_Mapping := Maps.Identity) return Natural is
    begin
        if Source'Length = 0 then
            if Pattern'Length = 0 then
                raise Pattern_Error;
            end if;
            return 0;
        end if;
        if Going = Forward then
            return Index (Source, Pattern, Source'First, Going, Mapping);
        end if;
        return Index (Source, Pattern, Source'Last, Going, Mapping);
    end Index;

    function Index (Source : String; Set : Maps.Character_Set; From : Positive;
                    Test : Membership := Inside; Going : Direction := Forward) return Natural is
    begin
        if Source'Length = 0 then
            return 0;
        end if;
        if From < Source'First or From > Source'Last then
            raise Index_Error;
        end if;
        if Going = Forward then
            for i in From .. Source'Last loop
                if Maps.Is_In (Source (i), Set) = (Test = Inside) then
                    return i;
                end if;
            end loop;
        else
            for i in reverse Source'First .. From loop
                if Maps.Is_In (Source (i), Set) = (Test = Inside) then
                    return i;
                end if;
            end loop;
        end if;
        return 0;
    end Index;

    function Index (Source : String; Set : Maps.Character_Set;
                    Test : Membership := Inside; Going : Direction := Forward) return Natural is
    begin
        if Source'Length = 0 then
            return 0;
        end if;
        if Going = Forward then
            return Index (Source, Set, Source'First, Test, Going);
        end if;
        return Index (Source, Set, Source'Last, Test, Going);
    end Index;

    function Index_Non_Blank (Source : String; From : Positive;
        Going : Direction := Forward) return Natural is
    begin
        return Index (Source, Maps.To_Set (Space), From, Outside, Going);
    end Index_Non_Blank;

    function Index_Non_Blank (Source : String; Going : Direction := Forward) return Natural is
    begin
        return Index (Source, Maps.To_Set (Space), Outside, Going);
    end Index_Non_Blank;

    function Count (Source, Pattern : String;
                    Mapping : Maps.Character_Mapping := Maps.Identity) return Natural is
        offset : Natural := 0;
        result : Natural := 0;
    begin
        if Pattern'Length = 0 then
            raise Pattern_Error;
        end if;
        while offset <= Source'Length - Pattern'Length loop
            if matches (Source, Pattern, offset, Mapping) then
                result := result + 1;
                offset := offset + Pattern'Length;
            else
                offset := offset + 1;
            end if;
        end loop;
        return result;
    end Count;

    function Count (Source : String; Set : Maps.Character_Set) return Natural is
        result : Natural := 0;
    begin
        for i in Source'Range loop
            if Maps.Is_In (Source (i), Set) then
                result := result + 1;
            end if;
        end loop;
        return result;
    end Count;

    procedure Find_Token (Source : String; Set : Maps.Character_Set; From : Positive;
                          Test : Membership; First : out Positive; Last : out Natural) is
        start : Natural;
    begin
        start := Index (Source, Set, From, Test);
        First := From;
        Last := 0;
        if start /= 0 then
            First := start;
            for i in start .. Source'Last loop
                exit when Maps.Is_In (Source (i), Set) /= (Test = Inside);
                Last := i;
            end loop;
        end if;
    end Find_Token;

    procedure Find_Token (Source : String; Set : Maps.Character_Set;
                          Test : Membership; First : out Positive; Last : out Natural) is
    begin
        Find_Token (Source, Set, Source'First, Test, First, Last);
    end Find_Token;

    function Translate (Source : String; Mapping : Maps.Character_Mapping) return String is
        result : String (1 .. Source'Length);
    begin
        for i in result'Range loop
            result (i) := Maps.Value (Mapping, Source (Source'First + (i - 1)));
        end loop;
        return result;
    end Translate;

    procedure Translate (Source : in out String; Mapping : Maps.Character_Mapping) is
    begin
        Source := Translate (Source, Mapping);
    end Translate;

    function Insert (Source : String; Before : Positive; New_Item : String) return String is
        prefixLength : Integer := Before - Source'First;
    begin
        if prefixLength < 0 or prefixLength > Source'Length then
            raise Index_Error;
        end if;
        return segment (Source, 0, prefixLength) & New_Item &
            segment (Source, prefixLength, Source'Length - prefixLength);
    end Insert;

    function Replace_Slice (Source : String; Low : Positive; High : Natural; By : String) return String is
        prefixLength : Natural := 0;
        suffixOffset : Natural := Source'Length;
    begin
        if Long_Integer (Low) > Long_Integer (Source'Last) + 1 or
            High < Source'First - 1 then
            raise Index_Error;
        end if;
        if High < Low then
            return Insert (Source, Low, By);
        end if;
        if Low > Source'First then
            prefixLength := Low - Source'First;
        end if;
        if High < Source'Last then
            suffixOffset := (High - Source'First) + 1;
        end if;
        return segment (Source, 0, prefixLength) & By &
            segment (Source, suffixOffset, Source'Length - suffixOffset);
    end Replace_Slice;

    function Overwrite (Source : String; Position : Positive; New_Item : String) return String is
        prefixLength : Integer := Position - Source'First;
        suffixOffset : Natural;
    begin
        if prefixLength < 0 or prefixLength > Source'Length then
            raise Index_Error;
        end if;
        if New_Item'Length >= Source'Length - prefixLength then
            return segment (Source, 0, prefixLength) & New_Item;
        end if;
        suffixOffset := prefixLength + New_Item'Length;
        return segment (Source, 0, prefixLength) & New_Item &
            segment (Source, suffixOffset, Source'Length - suffixOffset);
    end Overwrite;

    function Delete (Source : String; From : Positive; Through : Natural) return String is
    begin
        if Through < From then
            return segment (Source, 0, Source'Length);
        end if;
        return Replace_Slice (Source, From, Through, "");
    end Delete;

    function Trim (Source : String; Left, Right : Maps.Character_Set) return String is
        firstOffset : Natural := 0;
        endOffset : Natural := Source'Length;
    begin
        while firstOffset < endOffset loop
            exit when not Maps.Is_In (Source (Source'First + firstOffset), Left);
            firstOffset := firstOffset + 1;
        end loop;
        while endOffset > firstOffset loop
            exit when not Maps.Is_In (Source (Source'First + (endOffset - 1)), Right);
            endOffset := endOffset - 1;
        end loop;
        return segment (Source, firstOffset, endOffset - firstOffset);
    end Trim;

    function Trim (Source : String; Side : Trim_End) return String is
        leftSet : Maps.Character_Set := Maps.Null_Set;
        rightSet : Maps.Character_Set := Maps.Null_Set;
    begin
        if Side = Left or Side = Both then
            leftSet := Maps.To_Set (Space);
        end if;
        if Side = Right or Side = Both then
            rightSet := Maps.To_Set (Space);
        end if;
        return Trim (Source, leftSet, rightSet);
    end Trim;

    function Head (Source : String; Count : Natural; Pad : Character := Space) return String is
        result : String (1 .. Count) := (others => Pad);
        copyLength : Natural := Count;
    begin
        if Source'Length < copyLength then
            copyLength := Source'Length;
        end if;
        for i in 0 .. copyLength - 1 loop
            result (1 + i) := Source (Source'First + i);
        end loop;
        return result;
    end Head;

    function Tail (Source : String; Count : Natural; Pad : Character := Space) return String is
        result : String (1 .. Count) := (others => Pad);
        copyLength : Natural := Count;
    begin
        if Source'Length < copyLength then
            copyLength := Source'Length;
        end if;
        for i in 0 .. copyLength - 1 loop
            result (Count - i) := Source (Source'Last - i);
        end loop;
        return result;
    end Tail;

    procedure Insert (Source : in out String; Before : Positive; New_Item : String;
                      Drop : Truncation := Error) is
    begin
        Move (Insert (Source, Before, New_Item), Source, Drop);
    end Insert;

    procedure Replace_Slice (Source : in out String; Low : Positive; High : Natural; By : String;
                      Drop : Truncation := Error; Justify : Alignment := Left; Pad : Character := Space) is
    begin
        Move (Replace_Slice (Source, Low, High, By), Source, Drop, Justify, Pad);
    end Replace_Slice;

    procedure Overwrite (Source : in out String; Position : Positive; New_Item : String;
                      Drop : Truncation := Right) is
    begin
        Move (Overwrite (Source, Position, New_Item), Source, Drop);
    end Overwrite;

    procedure Delete (Source : in out String; From : Positive; Through : Natural;
                      Justify : Alignment := Left; Pad : Character := Space) is
    begin
        Move (Delete (Source, From, Through), Source, Justify => Justify, Pad => Pad);
    end Delete;

    procedure Trim (Source : in out String; Side : Trim_End;
                      Justify : Alignment := Left; Pad : Character := Space) is
    begin
        Move (Trim (Source, Side), Source, Justify => Justify, Pad => Pad);
    end Trim;

    procedure Trim (Source : in out String; Left, Right : Maps.Character_Set;
                      Justify : Alignment := Strings.Left; Pad : Character := Space) is
    begin
        Move (Trim (Source, Left, Right), Source, Justify => Justify, Pad => Pad);
    end Trim;

    procedure Head (Source : in out String; Count : Natural;
                      Justify : Alignment := Left; Pad : Character := Space) is
    begin
        Move (Head (Source, Count, Pad), Source, Justify => Justify, Pad => Pad);
    end Head;

    procedure Tail (Source : in out String; Count : Natural;
                      Justify : Alignment := Left; Pad : Character := Space) is
    begin
        Move (Tail (Source, Count, Pad), Source, Justify => Justify, Pad => Pad);
    end Tail;

    function "*" (Left : Natural; Right : Character) return String is
        result : String (1 .. Left) := (others => Right);
    begin
        return result;
    end "*";

    function "*" (Left : Natural; Right : String) return String is
        result : String (1 .. Left * Right'Length);
    begin
        if Right'Length /= 0 then
            for i in result'Range loop
                result (i) := Right (Right'First + ((i - 1) mod Right'Length));
            end loop;
        end if;
        return result;
    end "*";

end Ada.Strings.Fixed;
