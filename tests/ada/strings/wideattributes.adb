with Ada.Text_IO;
with WideAttributeViews;
procedure WideAttributes is
    type Unsigned is mod 2 ** 64;
    type Binary is delta 0.125 range -10.0 .. 10.0;
    type Decimal is delta 0.01 digits 6;
    type Words is (Alpha, Ωmega, 𐐀word);
    generic
        type T is private;
        with function Image (Arg : T) return Wide_String;
        with function Value (Arg : Wide_String) return T;
        with function Full_Image (Arg : T) return Wide_Wide_String;
        with function Full_Value (Arg : Wide_Wide_String) return T;
    procedure Round_Trip (Item : T);
    procedure Round_Trip (Item : T) is
    begin
        if Value (Image (Item)) /= Item or Full_Value (Full_Image (Item)) /= Item then
            raise Program_Error;
        end if;
    end Round_Trip;
    procedure Integers is new Round_Trip (Integer, Integer'Wide_Image, Integer'Wide_Value,
        Integer'Wide_Wide_Image, Integer'Wide_Wide_Value);
    procedure Modulars is new Round_Trip (Unsigned, Unsigned'Wide_Image, Unsigned'Wide_Value,
        Unsigned'Wide_Wide_Image, Unsigned'Wide_Wide_Value);
    procedure Reals is new Round_Trip (Float, Float'Wide_Image, Float'Wide_Value,
        Float'Wide_Wide_Image, Float'Wide_Wide_Value);
    procedure Fixeds is new Round_Trip (Binary, Binary'Wide_Image, Binary'Wide_Value,
        Binary'Wide_Wide_Image, Binary'Wide_Wide_Value);
    procedure Decimals is new Round_Trip (Decimal, Decimal'Wide_Image, Decimal'Wide_Value,
        Decimal'Wide_Wide_Image, Decimal'Wide_Wide_Value);
    procedure Characters is new Round_Trip (Wide_Character, Wide_Character'Wide_Image, Wide_Character'Wide_Value,
        Wide_Character'Wide_Wide_Image, Wide_Character'Wide_Wide_Value);
    procedure Full_Characters is new Round_Trip (Wide_Wide_Character, Wide_Wide_Character'Wide_Image, Wide_Wide_Character'Wide_Value,
        Wide_Wide_Character'Wide_Wide_Image, Wide_Wide_Character'Wide_Wide_Value);
    procedure Enumerations is new Round_Trip (Words, Words'Wide_Image, Words'Wide_Value,
        Words'Wide_Wide_Image, Words'Wide_Wide_Value);
    function Fixed_Image (Arg : Binary) return Wide_String renames Binary'Wide_Image;
    function Fixed_Value (Arg : Wide_String) return Binary renames Binary'Wide_Value;
    function Full_Image (Arg : Words) return Wide_Wide_String renames Words'Wide_Wide_Image;
    function Full_Value (Arg : Wide_Wide_String) return Words renames Words'Wide_Wide_Value;
    generic
        with function Image (Arg : Integer) return Wide_String is Integer'Wide_Image;
        with function Value (Arg : Wide_String) return Integer is Integer'Wide_Value;
    function Defaults (Item : Integer) return Integer;
    function Defaults (Item : Integer) return Integer is
    begin
        return Value (Image (Item));
    end Defaults;
    function From_Defaults is new Defaults;
    function From_Renamings is new Defaults (WideAttributeViews.Image, WideAttributeViews.Value);
    procedure Reject (Text : Wide_String) is
    begin
        declare
            Invalid : Words := Words'Wide_Value (Text);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end Reject;
    procedure Check is
        use WideAttributeViews;
        Image : Wide_String := WideAttributeViews.Image (4);
        Full : Wide_Wide_String := WideAttributeViews.Image (4);
    begin
        Integers (Integer'First);
        Integers (Integer'Last);
        Modulars (Unsigned'Last);
        Reals (1.5);
        Fixeds (-1.25);
        Decimals (1.25);
        Characters ('Ω');
        Characters (Wide_Character'Val (0));
        Full_Characters ('😀');
        Full_Characters (Wide_Wide_Character'Last);
        Enumerations (Ωmega);
        Enumerations (𐐀word);
        Reject ("[""0001040G""]word");
        Reject ("[""00110400""]word");
        Reject ("[""00010400""word");
        if Image /= " 4" or Full /= " 4" or WideAttributeViews.Value (Image) /= 4
            or WideAttributeViews.Value (Full) /= 4 or Base_Image (4) /= Image
            or From_Defaults (-4) /= -4 or From_Renamings (4) /= 4
            or Fixed_Value (Fixed_Image (-1.25)) /= -1.25
            or Full_Value (Full_Image (Ωmega)) /= Ωmega
            or Words'Wide_Width /= 16 or Words'Wide_Wide_Width /= 5
            or Words'Wide_Value (Wide_String'("[""00010428""]word")) /= 𐐀word then
            raise Program_Error;
        end if;
        begin
            declare
                Invalid : Integer := WideAttributeViews.Value (Wide_String'("bad"));
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("wide attribute profiles passed");
end WideAttributes;
