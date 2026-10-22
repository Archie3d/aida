package body Ada.Text_IO.Modular_IO is

    use Ada.Text_IO;

    Digit_Set : constant String (1 .. 16) := "0123456789ABCDEF";

    -- Room for 64 binary digits, a two-digit base and both delimiters.
    Text_Length : constant := 72;

    -- Item spelled out in the given base, right justified in Text and without
    -- any padding. First says where it starts. A nondecimal base is spelled
    -- as a based literal such as 16#FF#. Modular values have no minus sign.
    procedure Image_Of (Item  : in Num;
                        Base  : in Number_Base;
                        Text  : out String;
                        First : out Natural) is
        type Wide is mod 2 ** 64;
        Left     : Wide := Wide (Item);
        Digit    : Integer;
        Position : Natural := Text_Length + 1;
    begin
        if Base /= 10 then
            Position := Position - 1;
            Text (Position) := '#';
        end if;

        if Left = 0 then
            Position := Position - 1;
            Text (Position) := '0';
        end if;

        while Left /= 0 loop
            Digit := Integer (Left rem Wide (Base));
            Left := Left / Wide (Base);
            Position := Position - 1;
            Text (Position) := Digit_Set (Digit + 1);
        end loop;

        -- The base itself is written the same way round as the number, so its
        -- last digit is laid down first.
        if Base /= 10 then
            Position := Position - 1;
            Text (Position) := '#';
            Position := Position - 1;
            Text (Position) := Digit_Set (Base rem 10 + 1);
            if Base >= 10 then
                Position := Position - 1;
                Text (Position) := Digit_Set (Base / 10 + 1);
            end if;
        end if;

        First := Position;
    end Image_Of;

    procedure Put (Item  : in Num;
                   Width : in Field := Default_Width;
                   Base  : in Number_Base := Default_Base) is
        Text  : String (1 .. Text_Length);
        First : Natural;
    begin
        Image_Of (Item, Base, Text, First);
        for I in 1 .. Width - (Text_Length - First + 1) loop
            Ada.Text_IO.Put (' ');
        end loop;
        Ada.Text_IO.Put (Text (First .. Text_Length));
    end Put;

    procedure Put (File  : in File_Type;
                   Item  : in Num;
                   Width : in Field := Default_Width;
                   Base  : in Number_Base := Default_Base) is
        Text  : String (1 .. Text_Length);
        First : Natural;
    begin
        Image_Of (Item, Base, Text, First);
        for I in 1 .. Width - (Text_Length - First + 1) loop
            Ada.Text_IO.Put (File, ' ');
        end loop;
        Ada.Text_IO.Put (File, Text (First .. Text_Length));
    end Put;

    -- Invalid text or a value outside the instantiated range is Data_Error.
    procedure Store (Word : in String; Item : out Num) is
    begin
        Item := Num'Value (Word);
    exception
        when Constraint_Error =>
            raise Data_Error;
    end Store;

    procedure Get (Item : out Num; Width : in Field := 0) is
        Value : Ada.Text_IO.Scanning.Modular_Word;
    begin
        Value := Ada.Text_IO.Scanning.Get_Modular
            (Ada.Text_IO.Scanning.Modular_Word (Num'Base'Last), Width);
        Item := Num (Value);
    end Get;

    procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        Value : Ada.Text_IO.Scanning.Modular_Word;
    begin
        Value := Ada.Text_IO.Scanning.Get_Modular
            (File, Ada.Text_IO.Scanning.Modular_Word (Num'Base'Last), Width);
        Item := Num (Value);
    end Get;

    procedure Put (To : out String; Item : in Num; Base : in Number_Base := Default_Base) is
        Text : String (1 .. Text_Length);
        First : Natural;
        Length : Natural;
    begin
        Image_Of (Item, Base, Text, First);
        Length := Text_Length - First + 1;
        if Length > To'Length then
            raise Layout_Error;
        end if;
        To := (others => ' ');
        To (To'Last - Length + 1 .. To'Last) := Text (First .. Text_Length);
    end Put;

    procedure Get (From : in String; Item : out Num; Last : out Positive) is
        Count : Natural := Ada.Text_IO.Scanning.Modular_Token (From);
    begin
        Store (From (From'First .. From'First + Count - 1), Item);
        Last := From'First + Count - 1;
    end Get;

end Ada.Text_IO.Modular_IO;
