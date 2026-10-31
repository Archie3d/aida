with Ada.Strings.Wide_Fixed;
with Ada.Strings.Wide_Maps;
with Ada.Unchecked_Deallocation;
package body Ada.Strings.Wide_Unbounded is
    procedure Read_Unbounded (Stream : access Ada.Streams.Root_Stream_Type'Class;
                              Item : out Unbounded_Wide_String) is
    begin
        Set_Unbounded_Wide_String (Item, Wide_String'Input (Stream));
    end Read_Unbounded;

    procedure Write_Unbounded (Stream : access Ada.Streams.Root_Stream_Type'Class;
                               Item : Unbounded_Wide_String) is
    begin
        Wide_String'Output (Stream, To_Wide_String (Item));
    end Write_Unbounded;

    procedure Release is new Ada.Unchecked_Deallocation (Wide_String, Wide_String_Access);

    procedure Free (X : in out Wide_String_Access) is
    begin
        Release (X);
    end Free;

    procedure Adjust (Object : in out Storage) is
        Original : Wide_String_Access := Object.Data;
    begin
        -- Detach the shallow copy before any operation that can raise.
        Object.Data := null;
        if Original /= null then
            Object.Data := new Wide_String'(Original.all);
        end if;
    end Adjust;

    procedure Finalize (Object : in out Storage) is
    begin
        Free (Object.Data);
    end Finalize;

    function Length (Source : Unbounded_Wide_String) return Natural is
    begin
        if Source.Value.Data = null then
            return 0;
        end if;
        return Source.Value.Data.all'Length;
    end Length;

    function To_Unbounded_Wide_String (Source : Wide_String) return Unbounded_Wide_String is
        Result : Unbounded_Wide_String;
    begin
        if Source'Length > 0 then
            Result.Value.Data := new Wide_String'(Source);
        end if;
        return Result;
    end To_Unbounded_Wide_String;

    function To_Unbounded_Wide_String (Length : Natural) return Unbounded_Wide_String is
        Text : Wide_String (1 .. Length) := (others => Wide_Character'Val (0));
    begin
        return To_Unbounded_Wide_String (Text);
    end To_Unbounded_Wide_String;

    function To_Wide_String (Source : Unbounded_Wide_String) return Wide_String is
        Result : Wide_String (1 .. Length (Source));
    begin
        if Source.Value.Data /= null then
            Result := Source.Value.Data.all;
        end if;
        return Result;
    end To_Wide_String;

    procedure Set_Unbounded_Wide_String (Target : out Unbounded_Wide_String; Source : Wide_String) is
        Replacement : Wide_String_Access := null;
    begin
        if Source'Length > 0 then
            Replacement := new Wide_String'(Source);
        end if;
        Free (Target.Value.Data);
        Target.Value.Data := Replacement;
    end Set_Unbounded_Wide_String;

    function "&" (Left : Unbounded_Wide_String; Right : Unbounded_Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (To_Wide_String (Left) & To_Wide_String (Right));
    end "&";

    function "&" (Left : Unbounded_Wide_String; Right : Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (To_Wide_String (Left) & Right);
    end "&";

    function "&" (Left : Wide_String; Right : Unbounded_Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Left & To_Wide_String (Right));
    end "&";

    function "&" (Left : Unbounded_Wide_String; Right : Wide_Character) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (To_Wide_String (Left) & Right);
    end "&";

    function "&" (Left : Wide_Character; Right : Unbounded_Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Left & To_Wide_String (Right));
    end "&";

    procedure Append (Source : in out Unbounded_Wide_String; New_Item : Unbounded_Wide_String) is
    begin
        Source := Source & New_Item;
    end Append;

    procedure Append (Source : in out Unbounded_Wide_String; New_Item : Wide_String) is
    begin
        Source := Source & New_Item;
    end Append;

    procedure Append (Source : in out Unbounded_Wide_String; New_Item : Wide_Character) is
    begin
        Source := Source & New_Item;
    end Append;

    function Element (Source : Unbounded_Wide_String; Index : Positive) return Wide_Character is
        Text : Wide_String := To_Wide_String (Source);
    begin
        if Index > Text'Length then
            raise Index_Error;
        end if;
        return Text (Index);
    end Element;

    procedure Replace_Element (Source : in out Unbounded_Wide_String; Index : Positive; By : Wide_Character) is
    begin
        if Index > Length (Source) then
            raise Index_Error;
        end if;
        Source.Value.Data.all (Source.Value.Data.all'First + (Index - 1)) := By;
    end Replace_Element;

    function Slice (Source : Unbounded_Wide_String; Low : Positive; High : Natural) return Wide_String is
        Text : Wide_String := To_Wide_String (Source);
    begin
        if Long_Integer (Low) > Long_Integer (Text'Length) + 1 or High > Text'Length then
            raise Index_Error;
        end if;
        return Text (Low .. High);
    end Slice;
    function Unbounded_Slice (Source : Unbounded_Wide_String; Low : Positive;
        High : Natural) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Slice (Source, Low, High));
    end Unbounded_Slice;

    procedure Unbounded_Slice (Source : Unbounded_Wide_String; Target : out Unbounded_Wide_String;
                             Low : Positive; High : Natural) is
    begin
        Target := Unbounded_Slice (Source, Low, High);
    end Unbounded_Slice;

    function "=" (Left : Unbounded_Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) = To_Wide_String (Right);
    end "=";

    function "=" (Left : Unbounded_Wide_String; Right : Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) = Right;
    end "=";

    function "=" (Left : Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return Left = To_Wide_String (Right);
    end "=";

    function "<" (Left : Unbounded_Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) < To_Wide_String (Right);
    end "<";

    function "<" (Left : Unbounded_Wide_String; Right : Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) < Right;
    end "<";

    function "<" (Left : Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return Left < To_Wide_String (Right);
    end "<";

    function "<=" (Left : Unbounded_Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) <= To_Wide_String (Right);
    end "<=";

    function "<=" (Left : Unbounded_Wide_String; Right : Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) <= Right;
    end "<=";

    function "<=" (Left : Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return Left <= To_Wide_String (Right);
    end "<=";

    function ">" (Left : Unbounded_Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) > To_Wide_String (Right);
    end ">";

    function ">" (Left : Unbounded_Wide_String; Right : Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) > Right;
    end ">";

    function ">" (Left : Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return Left > To_Wide_String (Right);
    end ">";

    function ">=" (Left : Unbounded_Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) >= To_Wide_String (Right);
    end ">=";

    function ">=" (Left : Unbounded_Wide_String; Right : Wide_String) return Boolean is
    begin
        return To_Wide_String (Left) >= Right;
    end ">=";

    function ">=" (Left : Wide_String; Right : Unbounded_Wide_String) return Boolean is
    begin
        return Left >= To_Wide_String (Right);
    end ">=";

    function Index (Source : Unbounded_Wide_String; Pattern : Wide_String; From : Positive;
        Going : Direction := Forward;
                    Mapping : Wide_Maps.Wide_Character_Mapping := Wide_Maps.Identity) return Natural is
    begin
        return Wide_Fixed.Index (To_Wide_String (Source), Pattern, From, Going, Mapping);
    end Index;

    function Index (Source : Unbounded_Wide_String; Pattern : Wide_String; From : Positive;
        Going : Direction := Forward;
                    Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Natural is
    begin
        return Wide_Fixed.Index (To_Wide_String (Source), Pattern, From, Going, Mapping);
    end Index;

    function Index (Source : Unbounded_Wide_String; Pattern : Wide_String; Going : Direction := Forward;
                    Mapping : Wide_Maps.Wide_Character_Mapping := Wide_Maps.Identity) return Natural is
    begin
        return Wide_Fixed.Index (To_Wide_String (Source), Pattern, Going, Mapping);
    end Index;

    function Index (Source : Unbounded_Wide_String; Pattern : Wide_String; Going : Direction := Forward;
                    Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Natural is
    begin
        return Wide_Fixed.Index (To_Wide_String (Source), Pattern, Going, Mapping);
    end Index;

    function Index (Source : Unbounded_Wide_String; Set : Wide_Maps.Wide_Character_Set; From : Positive;
                    Test : Membership := Inside; Going : Direction := Forward) return Natural is
    begin
        return Wide_Fixed.Index (To_Wide_String (Source), Set, From, Test, Going);
    end Index;

    function Index (Source : Unbounded_Wide_String; Set : Wide_Maps.Wide_Character_Set;
                    Test : Membership := Inside; Going : Direction := Forward) return Natural is
    begin
        return Wide_Fixed.Index (To_Wide_String (Source), Set, Test, Going);
    end Index;

    function Index_Non_Blank (Source : Unbounded_Wide_String; From : Positive;
        Going : Direction := Forward) return Natural is
    begin
        return Wide_Fixed.Index_Non_Blank (To_Wide_String (Source), From, Going);
    end Index_Non_Blank;

    procedure Find_Token (Source : Unbounded_Wide_String; Set : Wide_Maps.Wide_Character_Set; From : Positive;
                          Test : Membership; First : out Positive; Last : out Natural) is
    begin
        Wide_Fixed.Find_Token (To_Wide_String (Source), Set, From, Test, First, Last);
    end Find_Token;

    function Index_Non_Blank (Source : Unbounded_Wide_String; Going : Direction := Forward) return Natural is
    begin
        return Wide_Fixed.Index_Non_Blank (To_Wide_String (Source), Going);
    end Index_Non_Blank;

    procedure Find_Token (Source : Unbounded_Wide_String; Set : Wide_Maps.Wide_Character_Set;
                          Test : Membership; First : out Positive; Last : out Natural) is
    begin
        Wide_Fixed.Find_Token (To_Wide_String (Source), Set, Test, First, Last);
    end Find_Token;

    function Count (Source : Unbounded_Wide_String; Pattern : Wide_String;
        Mapping : Wide_Maps.Wide_Character_Mapping := Wide_Maps.Identity) return Natural is
    begin
        return Wide_Fixed.Count (To_Wide_String (Source), Pattern, Mapping);
    end Count;

    function Count (Source : Unbounded_Wide_String; Pattern : Wide_String;
        Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Natural is
    begin
        return Wide_Fixed.Count (To_Wide_String (Source), Pattern, Mapping);
    end Count;

    function Count (Source : Unbounded_Wide_String; Set : Wide_Maps.Wide_Character_Set) return Natural is
    begin
        return Wide_Fixed.Count (To_Wide_String (Source), Set);
    end Count;

    function Translate (Source : Unbounded_Wide_String;
        Mapping : Wide_Maps.Wide_Character_Mapping) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Translate (To_Wide_String (Source), Mapping));
    end Translate;

    function Translate (Source : Unbounded_Wide_String;
        Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Translate (To_Wide_String (Source), Mapping));
    end Translate;

    procedure Translate (Source : in out Unbounded_Wide_String; Mapping : Wide_Maps.Wide_Character_Mapping) is
    begin
        Source := Translate (Source, Mapping);
    end Translate;

    procedure Translate (Source : in out Unbounded_Wide_String; Mapping : Wide_Maps.Wide_Character_Mapping_Function) is
    begin
        Source := Translate (Source, Mapping);
    end Translate;

    function Replace_Slice (Source : Unbounded_Wide_String; Low : Positive; High : Natural; By : Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Replace_Slice (To_Wide_String (Source), Low, High, By));
    end Replace_Slice;

    procedure Replace_Slice (Source : in out Unbounded_Wide_String; Low : Positive; High : Natural; By : Wide_String) is
    begin
        Source := Replace_Slice (Source, Low, High, By);
    end Replace_Slice;

    function Insert (Source : Unbounded_Wide_String; Before : Positive; New_Item : Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Insert (To_Wide_String (Source), Before, New_Item));
    end Insert;

    procedure Insert (Source : in out Unbounded_Wide_String; Before : Positive; New_Item : Wide_String) is
    begin
        Source := Insert (Source, Before, New_Item);
    end Insert;

    function Overwrite (Source : Unbounded_Wide_String; Position : Positive; New_Item : Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Overwrite (To_Wide_String (Source), Position, New_Item));
    end Overwrite;

    procedure Overwrite (Source : in out Unbounded_Wide_String; Position : Positive; New_Item : Wide_String) is
    begin
        Source := Overwrite (Source, Position, New_Item);
    end Overwrite;

    function Delete (Source : Unbounded_Wide_String; From : Positive; Through : Natural) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Delete (To_Wide_String (Source), From, Through));
    end Delete;

    procedure Delete (Source : in out Unbounded_Wide_String; From : Positive; Through : Natural) is
    begin
        Source := Delete (Source, From, Through);
    end Delete;

    function Trim (Source : Unbounded_Wide_String; Side : Trim_End) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Trim (To_Wide_String (Source), Side));
    end Trim;

    procedure Trim (Source : in out Unbounded_Wide_String; Side : Trim_End) is
    begin
        Source := Trim (Source, Side);
    end Trim;

    function Trim (Source : Unbounded_Wide_String; Left, Right : Wide_Maps.Wide_Character_Set) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Trim (To_Wide_String (Source), Left, Right));
    end Trim;

    procedure Trim (Source : in out Unbounded_Wide_String; Left, Right : Wide_Maps.Wide_Character_Set) is
    begin
        Source := Trim (Source, Left, Right);
    end Trim;

    function Head (Source : Unbounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space)
        return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Head (To_Wide_String (Source), Count, Pad));
    end Head;

    procedure Head (Source : in out Unbounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space) is
    begin
        Source := Head (Source, Count, Pad);
    end Head;

    function Tail (Source : Unbounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space)
        return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed.Tail (To_Wide_String (Source), Count, Pad));
    end Tail;

    procedure Tail (Source : in out Unbounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space) is
    begin
        Source := Tail (Source, Count, Pad);
    end Tail;

    function "*" (Left : Natural; Right : Wide_Character) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed."*" (Left, Right));
    end "*";

    function "*" (Left : Natural; Right : Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed."*" (Left, Right));
    end "*";

    function "*" (Left : Natural; Right : Unbounded_Wide_String) return Unbounded_Wide_String is
    begin
        return To_Unbounded_Wide_String (Wide_Fixed."*" (Left, To_Wide_String (Right)));
    end "*";

end Ada.Strings.Wide_Unbounded;
