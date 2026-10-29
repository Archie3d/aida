with Ada.Strings.Fixed;
with Ada.Strings.Maps;
with Ada.Unchecked_Deallocation;
package body Ada.Strings.Unbounded is
    procedure Read_Unbounded (Stream : access Ada.Streams.Root_Stream_Type'Class;
                              Item : out Unbounded_String) is
    begin
        Set_Unbounded_String (Item, String'Input (Stream));
    end Read_Unbounded;

    procedure Write_Unbounded (Stream : access Ada.Streams.Root_Stream_Type'Class;
                               Item : Unbounded_String) is
    begin
        String'Output (Stream, To_String (Item));
    end Write_Unbounded;

    procedure Release is new Ada.Unchecked_Deallocation (String, String_Access);

    procedure Free (X : in out String_Access) is
    begin
        Release (X);
    end Free;

    procedure Adjust (Object : in out Storage) is
        Original : String_Access := Object.Data;
    begin
        -- Detach the shallow copy before any operation that can raise.
        Object.Data := null;
        if Original /= null then
            Object.Data := new String'(Original.all);
        end if;
    end Adjust;

    procedure Finalize (Object : in out Storage) is
    begin
        Free (Object.Data);
    end Finalize;

    function Length (Source : Unbounded_String) return Natural is
    begin
        if Source.Value.Data = null then
            return 0;
        end if;
        return Source.Value.Data.all'Length;
    end Length;

    function To_Unbounded_String (Source : String) return Unbounded_String is
        Result : Unbounded_String;
    begin
        if Source'Length > 0 then
            Result.Value.Data := new String'(Source);
        end if;
        return Result;
    end To_Unbounded_String;

    function To_Unbounded_String (Length : Natural) return Unbounded_String is
        Text : String (1 .. Length) := (others => Character'Val (0));
    begin
        return To_Unbounded_String (Text);
    end To_Unbounded_String;

    function To_String (Source : Unbounded_String) return String is
        Result : String (1 .. Length (Source));
    begin
        if Source.Value.Data /= null then
            Result := Source.Value.Data.all;
        end if;
        return Result;
    end To_String;

    procedure Set_Unbounded_String (Target : out Unbounded_String; Source : String) is
        Replacement : String_Access := null;
    begin
        if Source'Length > 0 then
            Replacement := new String'(Source);
        end if;
        Free (Target.Value.Data);
        Target.Value.Data := Replacement;
    end Set_Unbounded_String;

    function "&" (Left : Unbounded_String; Right : Unbounded_String) return Unbounded_String is
    begin
        return To_Unbounded_String (To_String (Left) & To_String (Right));
    end "&";

    function "&" (Left : Unbounded_String; Right : String) return Unbounded_String is
    begin
        return To_Unbounded_String (To_String (Left) & Right);
    end "&";

    function "&" (Left : String; Right : Unbounded_String) return Unbounded_String is
    begin
        return To_Unbounded_String (Left & To_String (Right));
    end "&";

    function "&" (Left : Unbounded_String; Right : Character) return Unbounded_String is
    begin
        return To_Unbounded_String (To_String (Left) & Right);
    end "&";

    function "&" (Left : Character; Right : Unbounded_String) return Unbounded_String is
    begin
        return To_Unbounded_String (Left & To_String (Right));
    end "&";

    procedure Append (Source : in out Unbounded_String; New_Item : Unbounded_String) is
    begin
        Source := Source & New_Item;
    end Append;

    procedure Append (Source : in out Unbounded_String; New_Item : String) is
    begin
        Source := Source & New_Item;
    end Append;

    procedure Append (Source : in out Unbounded_String; New_Item : Character) is
    begin
        Source := Source & New_Item;
    end Append;

    function Element (Source : Unbounded_String; Index : Positive) return Character is
        Text : String := To_String (Source);
    begin
        if Index > Text'Length then
            raise Index_Error;
        end if;
        return Text (Index);
    end Element;

    procedure Replace_Element (Source : in out Unbounded_String; Index : Positive; By : Character) is
    begin
        if Index > Length (Source) then
            raise Index_Error;
        end if;
        Source.Value.Data.all (Source.Value.Data.all'First + (Index - 1)) := By;
    end Replace_Element;

    function Slice (Source : Unbounded_String; Low : Positive; High : Natural) return String is
        Text : String := To_String (Source);
    begin
        if Long_Integer (Low) > Long_Integer (Text'Length) + 1 or High > Text'Length then
            raise Index_Error;
        end if;
        return Text (Low .. High);
    end Slice;
    function Unbounded_Slice (Source : Unbounded_String; Low : Positive;
        High : Natural) return Unbounded_String is
    begin
        return To_Unbounded_String (Slice (Source, Low, High));
    end Unbounded_Slice;

    procedure Unbounded_Slice (Source : Unbounded_String; Target : out Unbounded_String;
                             Low : Positive; High : Natural) is
    begin
        Target := Unbounded_Slice (Source, Low, High);
    end Unbounded_Slice;

    function "=" (Left : Unbounded_String; Right : Unbounded_String) return Boolean is
    begin
        return To_String (Left) = To_String (Right);
    end "=";

    function "=" (Left : Unbounded_String; Right : String) return Boolean is
    begin
        return To_String (Left) = Right;
    end "=";

    function "=" (Left : String; Right : Unbounded_String) return Boolean is
    begin
        return Left = To_String (Right);
    end "=";

    function "<" (Left : Unbounded_String; Right : Unbounded_String) return Boolean is
    begin
        return To_String (Left) < To_String (Right);
    end "<";

    function "<" (Left : Unbounded_String; Right : String) return Boolean is
    begin
        return To_String (Left) < Right;
    end "<";

    function "<" (Left : String; Right : Unbounded_String) return Boolean is
    begin
        return Left < To_String (Right);
    end "<";

    function "<=" (Left : Unbounded_String; Right : Unbounded_String) return Boolean is
    begin
        return To_String (Left) <= To_String (Right);
    end "<=";

    function "<=" (Left : Unbounded_String; Right : String) return Boolean is
    begin
        return To_String (Left) <= Right;
    end "<=";

    function "<=" (Left : String; Right : Unbounded_String) return Boolean is
    begin
        return Left <= To_String (Right);
    end "<=";

    function ">" (Left : Unbounded_String; Right : Unbounded_String) return Boolean is
    begin
        return To_String (Left) > To_String (Right);
    end ">";

    function ">" (Left : Unbounded_String; Right : String) return Boolean is
    begin
        return To_String (Left) > Right;
    end ">";

    function ">" (Left : String; Right : Unbounded_String) return Boolean is
    begin
        return Left > To_String (Right);
    end ">";

    function ">=" (Left : Unbounded_String; Right : Unbounded_String) return Boolean is
    begin
        return To_String (Left) >= To_String (Right);
    end ">=";

    function ">=" (Left : Unbounded_String; Right : String) return Boolean is
    begin
        return To_String (Left) >= Right;
    end ">=";

    function ">=" (Left : String; Right : Unbounded_String) return Boolean is
    begin
        return Left >= To_String (Right);
    end ">=";

    function Index (Source : Unbounded_String; Pattern : String; From : Positive;
        Going : Direction := Forward;
                    Mapping : Maps.Character_Mapping := Maps.Identity) return Natural is
    begin
        return Fixed.Index (To_String (Source), Pattern, From, Going, Mapping);
    end Index;

    function Index (Source : Unbounded_String; Pattern : String; Going : Direction := Forward;
                    Mapping : Maps.Character_Mapping := Maps.Identity) return Natural is
    begin
        return Fixed.Index (To_String (Source), Pattern, Going, Mapping);
    end Index;

    function Index (Source : Unbounded_String; Set : Maps.Character_Set; From : Positive;
                    Test : Membership := Inside; Going : Direction := Forward) return Natural is
    begin
        return Fixed.Index (To_String (Source), Set, From, Test, Going);
    end Index;

    function Index (Source : Unbounded_String; Set : Maps.Character_Set;
                    Test : Membership := Inside; Going : Direction := Forward) return Natural is
    begin
        return Fixed.Index (To_String (Source), Set, Test, Going);
    end Index;

    function Index_Non_Blank (Source : Unbounded_String; From : Positive;
        Going : Direction := Forward) return Natural is
    begin
        return Fixed.Index_Non_Blank (To_String (Source), From, Going);
    end Index_Non_Blank;

    procedure Find_Token (Source : Unbounded_String; Set : Maps.Character_Set; From : Positive;
                          Test : Membership; First : out Positive; Last : out Natural) is
    begin
        Fixed.Find_Token (To_String (Source), Set, From, Test, First, Last);
    end Find_Token;

    function Index_Non_Blank (Source : Unbounded_String; Going : Direction := Forward) return Natural is
    begin
        return Fixed.Index_Non_Blank (To_String (Source), Going);
    end Index_Non_Blank;

    procedure Find_Token (Source : Unbounded_String; Set : Maps.Character_Set;
                          Test : Membership; First : out Positive; Last : out Natural) is
    begin
        Fixed.Find_Token (To_String (Source), Set, Test, First, Last);
    end Find_Token;

    function Count (Source : Unbounded_String; Pattern : String;
        Mapping : Maps.Character_Mapping := Maps.Identity) return Natural is
    begin
        return Fixed.Count (To_String (Source), Pattern, Mapping);
    end Count;

    function Count (Source : Unbounded_String; Set : Maps.Character_Set) return Natural is
    begin
        return Fixed.Count (To_String (Source), Set);
    end Count;

    function Translate (Source : Unbounded_String;
        Mapping : Maps.Character_Mapping) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Translate (To_String (Source), Mapping));
    end Translate;

    procedure Translate (Source : in out Unbounded_String; Mapping : Maps.Character_Mapping) is
    begin
        Source := Translate (Source, Mapping);
    end Translate;

    function Replace_Slice (Source : Unbounded_String; Low : Positive; High : Natural; By : String) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Replace_Slice (To_String (Source), Low, High, By));
    end Replace_Slice;

    procedure Replace_Slice (Source : in out Unbounded_String; Low : Positive; High : Natural; By : String) is
    begin
        Source := Replace_Slice (Source, Low, High, By);
    end Replace_Slice;

    function Insert (Source : Unbounded_String; Before : Positive; New_Item : String) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Insert (To_String (Source), Before, New_Item));
    end Insert;

    procedure Insert (Source : in out Unbounded_String; Before : Positive; New_Item : String) is
    begin
        Source := Insert (Source, Before, New_Item);
    end Insert;

    function Overwrite (Source : Unbounded_String; Position : Positive; New_Item : String) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Overwrite (To_String (Source), Position, New_Item));
    end Overwrite;

    procedure Overwrite (Source : in out Unbounded_String; Position : Positive; New_Item : String) is
    begin
        Source := Overwrite (Source, Position, New_Item);
    end Overwrite;

    function Delete (Source : Unbounded_String; From : Positive; Through : Natural) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Delete (To_String (Source), From, Through));
    end Delete;

    procedure Delete (Source : in out Unbounded_String; From : Positive; Through : Natural) is
    begin
        Source := Delete (Source, From, Through);
    end Delete;

    function Trim (Source : Unbounded_String; Side : Trim_End) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Trim (To_String (Source), Side));
    end Trim;

    procedure Trim (Source : in out Unbounded_String; Side : Trim_End) is
    begin
        Source := Trim (Source, Side);
    end Trim;

    function Trim (Source : Unbounded_String; Left, Right : Maps.Character_Set) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Trim (To_String (Source), Left, Right));
    end Trim;

    procedure Trim (Source : in out Unbounded_String; Left, Right : Maps.Character_Set) is
    begin
        Source := Trim (Source, Left, Right);
    end Trim;

    function Head (Source : Unbounded_String; Count : Natural; Pad : Character := Space)
        return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Head (To_String (Source), Count, Pad));
    end Head;

    procedure Head (Source : in out Unbounded_String; Count : Natural; Pad : Character := Space) is
    begin
        Source := Head (Source, Count, Pad);
    end Head;

    function Tail (Source : Unbounded_String; Count : Natural; Pad : Character := Space)
        return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed.Tail (To_String (Source), Count, Pad));
    end Tail;

    procedure Tail (Source : in out Unbounded_String; Count : Natural; Pad : Character := Space) is
    begin
        Source := Tail (Source, Count, Pad);
    end Tail;

    function "*" (Left : Natural; Right : Character) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed."*" (Left, Right));
    end "*";

    function "*" (Left : Natural; Right : String) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed."*" (Left, Right));
    end "*";

    function "*" (Left : Natural; Right : Unbounded_String) return Unbounded_String is
    begin
        return To_Unbounded_String (Fixed."*" (Left, To_String (Right)));
    end "*";

end Ada.Strings.Unbounded;
