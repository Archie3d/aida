with Ada.Strings; use Ada.Strings;
with Ada.Strings.Wide_Maps; use Ada.Strings.Wide_Maps;
with Ada.Wide_Text_IO;
procedure wideStringMaps is
    procedure check (condition : Boolean; message : Wide_String) is
    begin
        if not condition then
            Ada.Wide_Text_IO.Put_Line (message);
            raise Program_Error;
        end if;
    end check;
    span : Wide_Character_Range := ('b', 'd');
    nullSpan : Wide_Character_Range := ('z', 'a');
    a : Wide_Character_Set := To_Set ("acba");
    b : Wide_Character_Set := To_Set (span);
    defaultSet : Wide_Character_Set;
    defaultMap : Wide_Character_Mapping;
    m : Wide_Character_Mapping := To_Mapping ("cba", "XXa");
    fromText : Wide_String (7 .. 8) := "AZ";
    toText : Wide_String (20 .. 21) := "az";
begin
    check (a = To_Set ("abc") and a /= b, "set equality");
    check (defaultSet = Null_Set and To_Sequence (Null_Set) = "", "empty set");
    check (To_Sequence (a) = "abc", "sorted unique sequence");
    check (To_Sequence (a and b) = "bc", "intersection");
    check (To_Sequence (a or b) = "abcd", "union");
    check (To_Sequence (a xor b) = "ad", "symmetric difference");
    check (To_Sequence (a - b) = "a", "difference");
    check (not Is_In ('a', not a) and Is_In (Wide_Character'Last, not a), "complement");
    check (To_Set ('b') <= a and not Is_Subset (b, a), "subset");
    check (To_Set (nullSpan) = Null_Set, "null range");
    declare
        ranges : Wide_Character_Ranges := To_Ranges (To_Set ("abcdefxz"));
        none : Wide_Character_Ranges := To_Ranges (Null_Set);
        allRanges : Wide_Character_Ranges := To_Ranges (not Null_Set);
    begin
        check (ranges'First = 1 and ranges'Length = 3, "minimal ranges");
        check (ranges (1).Low = 'a' and ranges (1).High = 'f', "merged range");
        check (To_Set (ranges) = To_Set ("abcdefxz"), "range round trip");
        check (none'Length = 0 and To_Set (none) = Null_Set, "empty ranges");
        check (allRanges'Length = 1 and allRanges (1).Low = Wide_Character'First and
            allRanges (1).High = Wide_Character'Last, "full range endpoints");
    end;
    check (To_Domain (m) = "bc" and To_Range (m) = "XX", "domain excludes fixed points");
    check (Value (m, 'a') = 'a' and Value (m, 'b') = 'X', "mapping value");
    check (To_Domain (Identity) = "" and To_Range (defaultMap) = "", "identity");
    m := To_Mapping (fromText, toText);
    check (Value (m, 'A') = 'a' and Value (m, 'Z') = 'z', "different bounds");
    for c in Wide_Character range Wide_Character'Val (0) .. Wide_Character'Val (255) loop
        check (Value (Identity, c) = c, "identity all characters");
    end loop;
    begin
        m := To_Mapping ("a", "");
        raise Program_Error;
    exception
        when Translation_Error => null;
    end;
    begin
        m := To_Mapping ("aa", "ab");
        raise Program_Error;
    exception
        when Translation_Error => null;
    end;
    check (Value (m, 'Z') = 'z', "failure preserves destination");
    Ada.Wide_Text_IO.Put_Line ("wideStringMaps: passed");
end wideStringMaps;
