with Ada.Strings.Wide_Fixed;
with Ada.Strings.Wide_Maps;
package body Ada.Strings.Wide_Bounded is
    package body Generic_Bounded_Length is
        function singleCharacter (item : Wide_Character) return Wide_String is
            result : Wide_String (1 .. 1) := (others => item);
        begin
            return result;
        end singleCharacter;

        function Length (Source : Bounded_Wide_String) return Length_Range is
        begin
            return Source.length;
        end Length;

        function To_Bounded_Wide_String (Source : Wide_String; Drop : Truncation := Error) return Bounded_Wide_String is
            result : Bounded_Wide_String;
            offset : Natural := 0;
        begin
            if Source'Length > Max_Length then
                if Drop = Error then
                    raise Length_Error;
                end if;
                result.length := Max_Length;
                if Drop = Left then
                    offset := Source'Length - Max_Length;
                end if;
            else
                result.length := Source'Length;
            end if;
            for i in 1 .. result.length loop
                result.data (i) := Source (Source'First + (offset + (i - 1)));
            end loop;
            return result;
        end To_Bounded_Wide_String;

        function To_Wide_String (Source : Bounded_Wide_String) return Wide_String is
        begin
            return Source.data (1 .. Source.length);
        end To_Wide_String;

        procedure Set_Bounded_Wide_String (Target : out Bounded_Wide_String; Source : Wide_String;
                                      Drop : Truncation := Error) is
        begin
            Target := To_Bounded_Wide_String (Source, Drop);
        end Set_Bounded_Wide_String;

        function concatenate (leftText, rightText : Wide_String; drop : Truncation) return Bounded_Wide_String is
            result : Bounded_Wide_String;
            total : Long_Integer := Long_Integer (leftText'Length) + Long_Integer (rightText'Length);
            offset : Long_Integer := 0;
            position : Long_Integer;
        begin
            if total > Long_Integer (Max_Length) then
                if drop = Error then
                    raise Length_Error;
                end if;
                result.length := Max_Length;
                if drop = Left then
                    offset := total - Long_Integer (Max_Length);
                end if;
            else
                result.length := Integer (total);
            end if;
            for i in 1 .. result.length loop
                position := offset + Long_Integer (i - 1);
                if position < Long_Integer (leftText'Length) then
                    result.data (i) := leftText (leftText'First + Integer (position));
                else
                    result.data (i) := rightText (rightText'First +
                        Integer (position - Long_Integer (leftText'Length)));
                end if;
            end loop;
            return result;
        end concatenate;

        function Append (Left : Bounded_Wide_String; Right : Bounded_Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return concatenate (To_Wide_String (Left), To_Wide_String (Right), Drop);
        end Append;

        function "&" (Left : Bounded_Wide_String; Right : Bounded_Wide_String) return Bounded_Wide_String is
        begin
            return Append (Left, Right);
        end "&";

        function Append (Left : Bounded_Wide_String; Right : Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return concatenate (To_Wide_String (Left), Right, Drop);
        end Append;

        function "&" (Left : Bounded_Wide_String; Right : Wide_String) return Bounded_Wide_String is
        begin
            return Append (Left, Right);
        end "&";

        function Append (Left : Wide_String; Right : Bounded_Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return concatenate (Left, To_Wide_String (Right), Drop);
        end Append;

        function "&" (Left : Wide_String; Right : Bounded_Wide_String) return Bounded_Wide_String is
        begin
            return Append (Left, Right);
        end "&";

        function Append (Left : Bounded_Wide_String; Right : Wide_Character;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return concatenate (To_Wide_String (Left), singleCharacter (Right), Drop);
        end Append;

        function "&" (Left : Bounded_Wide_String; Right : Wide_Character) return Bounded_Wide_String is
        begin
            return Append (Left, Right);
        end "&";

        function Append (Left : Wide_Character; Right : Bounded_Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return concatenate (singleCharacter (Left), To_Wide_String (Right), Drop);
        end Append;

        function "&" (Left : Wide_Character; Right : Bounded_Wide_String) return Bounded_Wide_String is
        begin
            return Append (Left, Right);
        end "&";

        procedure Append (Source : in out Bounded_Wide_String; New_Item : Bounded_Wide_String;
            Drop : Truncation := Error) is
        begin
            Source := Append (Source, New_Item, Drop);
        end Append;

        procedure Append (Source : in out Bounded_Wide_String; New_Item : Wide_String; Drop : Truncation := Error) is
        begin
            Source := Append (Source, New_Item, Drop);
        end Append;

        procedure Append (Source : in out Bounded_Wide_String; New_Item : Wide_Character; Drop : Truncation := Error) is
        begin
            Source := Append (Source, New_Item, Drop);
        end Append;

        function Element (Source : Bounded_Wide_String; Index : Positive) return Wide_Character is
        begin
            if Index > Source.length then
                raise Index_Error;
            end if;
            return Source.data (Index);
        end Element;

        procedure Replace_Element (Source : in out Bounded_Wide_String; Index : Positive; By : Wide_Character) is
        begin
            if Index > Source.length then
                raise Index_Error;
            end if;
            Source.data (Index) := By;
        end Replace_Element;

        function Slice (Source : Bounded_Wide_String; Low : Positive; High : Natural) return Wide_String is
        begin
            if Long_Integer (Low) > Long_Integer (Source.length) + 1 or High > Source.length then
                raise Index_Error;
            end if;
            return Source.data (Low .. High);
        end Slice;

        function Bounded_Slice (Source : Bounded_Wide_String; Low : Positive;
            High : Natural) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Slice (Source, Low, High));
        end Bounded_Slice;

        procedure Bounded_Slice (Source : Bounded_Wide_String; Target : out Bounded_Wide_String;
                                 Low : Positive; High : Natural) is
        begin
            Target := Bounded_Slice (Source, Low, High);
        end Bounded_Slice;

        function "=" (Left : Bounded_Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) = To_Wide_String (Right);
        end "=";

        function "=" (Left : Bounded_Wide_String; Right : Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) = Right;
        end "=";

        function "=" (Left : Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return Left = To_Wide_String (Right);
        end "=";

        function "<" (Left : Bounded_Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) < To_Wide_String (Right);
        end "<";

        function "<" (Left : Bounded_Wide_String; Right : Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) < Right;
        end "<";

        function "<" (Left : Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return Left < To_Wide_String (Right);
        end "<";

        function "<=" (Left : Bounded_Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) <= To_Wide_String (Right);
        end "<=";

        function "<=" (Left : Bounded_Wide_String; Right : Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) <= Right;
        end "<=";

        function "<=" (Left : Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return Left <= To_Wide_String (Right);
        end "<=";

        function ">" (Left : Bounded_Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) > To_Wide_String (Right);
        end ">";

        function ">" (Left : Bounded_Wide_String; Right : Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) > Right;
        end ">";

        function ">" (Left : Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return Left > To_Wide_String (Right);
        end ">";

        function ">=" (Left : Bounded_Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) >= To_Wide_String (Right);
        end ">=";

        function ">=" (Left : Bounded_Wide_String; Right : Wide_String) return Boolean is
        begin
            return To_Wide_String (Left) >= Right;
        end ">=";

        function ">=" (Left : Wide_String; Right : Bounded_Wide_String) return Boolean is
        begin
            return Left >= To_Wide_String (Right);
        end ">=";

        function Index (Source : Bounded_Wide_String; Pattern : Wide_String; From : Positive;
            Going : Direction := Forward;
                        Mapping : Wide_Maps.Wide_Character_Mapping := Wide_Maps.Identity) return Natural is
        begin
            return Wide_Fixed.Index (To_Wide_String (Source), Pattern, From, Going, Mapping);
        end Index;

        function Index (Source : Bounded_Wide_String; Pattern : Wide_String; From : Positive;
            Going : Direction := Forward;
                        Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Natural is
        begin
            return Wide_Fixed.Index (To_Wide_String (Source), Pattern, From, Going, Mapping);
        end Index;

        function Index (Source : Bounded_Wide_String; Pattern : Wide_String; Going : Direction := Forward;
                        Mapping : Wide_Maps.Wide_Character_Mapping := Wide_Maps.Identity) return Natural is
        begin
            return Wide_Fixed.Index (To_Wide_String (Source), Pattern, Going, Mapping);
        end Index;

        function Index (Source : Bounded_Wide_String; Pattern : Wide_String; Going : Direction := Forward;
                        Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Natural is
        begin
            return Wide_Fixed.Index (To_Wide_String (Source), Pattern, Going, Mapping);
        end Index;

        function Index (Source : Bounded_Wide_String; Set : Wide_Maps.Wide_Character_Set; From : Positive;
                        Test : Membership := Inside; Going : Direction := Forward) return Natural is
        begin
            return Wide_Fixed.Index (To_Wide_String (Source), Set, From, Test, Going);
        end Index;

        function Index (Source : Bounded_Wide_String; Set : Wide_Maps.Wide_Character_Set;
                        Test : Membership := Inside; Going : Direction := Forward) return Natural is
        begin
            return Wide_Fixed.Index (To_Wide_String (Source), Set, Test, Going);
        end Index;

        function Index_Non_Blank (Source : Bounded_Wide_String; From : Positive;
            Going : Direction := Forward) return Natural is
        begin
            return Wide_Fixed.Index_Non_Blank (To_Wide_String (Source), From, Going);
        end Index_Non_Blank;

        procedure Find_Token (Source : Bounded_Wide_String; Set : Wide_Maps.Wide_Character_Set; From : Positive;
                              Test : Membership; First : out Positive; Last : out Natural) is
        begin
            Wide_Fixed.Find_Token (To_Wide_String (Source), Set, From, Test, First, Last);
        end Find_Token;

        function Index_Non_Blank (Source : Bounded_Wide_String; Going : Direction := Forward) return Natural is
        begin
            return Wide_Fixed.Index_Non_Blank (To_Wide_String (Source), Going);
        end Index_Non_Blank;

        procedure Find_Token (Source : Bounded_Wide_String; Set : Wide_Maps.Wide_Character_Set;
                              Test : Membership; First : out Positive; Last : out Natural) is
        begin
            Wide_Fixed.Find_Token (To_Wide_String (Source), Set, Test, First, Last);
        end Find_Token;

        function Count (Source : Bounded_Wide_String; Pattern : Wide_String;
            Mapping : Wide_Maps.Wide_Character_Mapping := Wide_Maps.Identity) return Natural is
        begin
            return Wide_Fixed.Count (To_Wide_String (Source), Pattern, Mapping);
        end Count;

        function Count (Source : Bounded_Wide_String; Pattern : Wide_String;
            Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Natural is
        begin
            return Wide_Fixed.Count (To_Wide_String (Source), Pattern, Mapping);
        end Count;

        function Count (Source : Bounded_Wide_String; Set : Wide_Maps.Wide_Character_Set) return Natural is
        begin
            return Wide_Fixed.Count (To_Wide_String (Source), Set);
        end Count;

        function Translate (Source : Bounded_Wide_String;
            Mapping : Wide_Maps.Wide_Character_Mapping) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Translate (To_Wide_String (Source), Mapping));
        end Translate;

        function Translate (Source : Bounded_Wide_String;
            Mapping : Wide_Maps.Wide_Character_Mapping_Function) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Translate (To_Wide_String (Source), Mapping));
        end Translate;

        procedure Translate (Source : in out Bounded_Wide_String; Mapping : Wide_Maps.Wide_Character_Mapping) is
        begin
            Source := Translate (Source, Mapping);
        end Translate;

        procedure Translate (Source : in out Bounded_Wide_String; Mapping : Wide_Maps.Wide_Character_Mapping_Function) is
        begin
            Source := Translate (Source, Mapping);
        end Translate;

        function Replace_Slice (Source : Bounded_Wide_String; Low : Positive; High : Natural; By : Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Replace_Slice (To_Wide_String (Source), Low, High, By), Drop);
        end Replace_Slice;

        procedure Replace_Slice (Source : in out Bounded_Wide_String; Low : Positive; High : Natural; By : Wide_String;
            Drop : Truncation := Error) is
        begin
            Source := Replace_Slice (Source, Low, High, By, Drop);
        end Replace_Slice;

        function Insert (Source : Bounded_Wide_String; Before : Positive; New_Item : Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Insert (To_Wide_String (Source), Before, New_Item), Drop);
        end Insert;

        procedure Insert (Source : in out Bounded_Wide_String; Before : Positive; New_Item : Wide_String;
            Drop : Truncation := Error) is
        begin
            Source := Insert (Source, Before, New_Item, Drop);
        end Insert;

        function Overwrite (Source : Bounded_Wide_String; Position : Positive; New_Item : Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Overwrite (To_Wide_String (Source), Position, New_Item), Drop);
        end Overwrite;

        procedure Overwrite (Source : in out Bounded_Wide_String; Position : Positive; New_Item : Wide_String;
            Drop : Truncation := Error) is
        begin
            Source := Overwrite (Source, Position, New_Item, Drop);
        end Overwrite;

        function Delete (Source : Bounded_Wide_String; From : Positive; Through : Natural) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Delete (To_Wide_String (Source), From, Through));
        end Delete;

        procedure Delete (Source : in out Bounded_Wide_String; From : Positive; Through : Natural) is
        begin
            Source := Delete (Source, From, Through);
        end Delete;

        function Trim (Source : Bounded_Wide_String; Side : Trim_End) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Trim (To_Wide_String (Source), Side));
        end Trim;

        procedure Trim (Source : in out Bounded_Wide_String; Side : Trim_End) is
        begin
            Source := Trim (Source, Side);
        end Trim;

        function Trim (Source : Bounded_Wide_String; Left, Right : Wide_Maps.Wide_Character_Set) return Bounded_Wide_String is
        begin
            return To_Bounded_Wide_String (Wide_Fixed.Trim (To_Wide_String (Source), Left, Right));
        end Trim;

        procedure Trim (Source : in out Bounded_Wide_String; Left, Right : Wide_Maps.Wide_Character_Set) is
        begin
            Source := Trim (Source, Left, Right);
        end Trim;

        function Head (Source : Bounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space;
            Drop : Truncation := Error) return Bounded_Wide_String is
            result : Bounded_Wide_String;
            offset : Natural := 0;
            position : Natural;
            sourcePosition : Integer;
        begin
            if Count > Max_Length then
                if Drop = Error then
                    raise Length_Error;
                end if;
                result.length := Max_Length;
                if Drop = Left then
                    offset := Count - Max_Length;
                end if;
            else
                result.length := Count;
            end if;
            for i in 1 .. result.length loop
                position := offset + i;
                sourcePosition := position;
                if sourcePosition >= 1 and sourcePosition <= Source.length then
                    result.data (i) := Source.data (sourcePosition);
                else
                    result.data (i) := Pad;
                end if;
            end loop;
            return result;
        end Head;

        procedure Head (Source : in out Bounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space;
            Drop : Truncation := Error) is
        begin
            Source := Head (Source, Count, Pad, Drop);
        end Head;

        function Tail (Source : Bounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space;
            Drop : Truncation := Error) return Bounded_Wide_String is
            result : Bounded_Wide_String;
            offset : Natural := 0;
            position : Natural;
            sourcePosition : Integer;
        begin
            if Count > Max_Length then
                if Drop = Error then
                    raise Length_Error;
                end if;
                result.length := Max_Length;
                if Drop = Left then
                    offset := Count - Max_Length;
                end if;
            else
                result.length := Count;
            end if;
            for i in 1 .. result.length loop
                position := offset + i;
                sourcePosition := Source.length - (Count - position);
                if sourcePosition >= 1 and sourcePosition <= Source.length then
                    result.data (i) := Source.data (sourcePosition);
                else
                    result.data (i) := Pad;
                end if;
            end loop;
            return result;
        end Tail;

        procedure Tail (Source : in out Bounded_Wide_String; Count : Natural; Pad : Wide_Character := Wide_Space;
            Drop : Truncation := Error) is
        begin
            Source := Tail (Source, Count, Pad, Drop);
        end Tail;

        function Replicate (Count : Natural; Item : Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
            result : Bounded_Wide_String;
            total : Long_Integer := Long_Integer (Count) * Long_Integer (Item'Length);
            offset : Long_Integer := 0;
            position : Natural;
        begin
            if total > Long_Integer (Max_Length) then
                if Drop = Error then
                    raise Length_Error;
                end if;
                result.length := Max_Length;
                if Drop = Left then
                    offset := total - Long_Integer (Max_Length);
                end if;
            else
                result.length := Integer (total);
            end if;
            for i in 1 .. result.length loop
                position := Integer ((offset + Long_Integer (i - 1)) mod Long_Integer (Item'Length));
                result.data (i) := Item (Item'First + position);
            end loop;
            return result;
        end Replicate;

        function Replicate (Count : Natural; Item : Wide_Character;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return Replicate (Count, singleCharacter (Item), Drop);
        end Replicate;

        function Replicate (Count : Natural; Item : Bounded_Wide_String;
            Drop : Truncation := Error) return Bounded_Wide_String is
        begin
            return Replicate (Count, To_Wide_String (Item), Drop);
        end Replicate;

        function "*" (Left : Natural; Right : Wide_Character) return Bounded_Wide_String is
        begin
            return Replicate (Left, Right);
        end "*";

        function "*" (Left : Natural; Right : Wide_String) return Bounded_Wide_String is
        begin
            return Replicate (Left, Right);
        end "*";

        function "*" (Left : Natural; Right : Bounded_Wide_String) return Bounded_Wide_String is
        begin
            return Replicate (Left, Right);
        end "*";

    end Generic_Bounded_Length;
end Ada.Strings.Wide_Bounded;
