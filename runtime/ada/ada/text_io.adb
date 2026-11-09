with Ada.Text_IO.Scanning;
package body Ada.Text_IO is
    procedure Read_Line (File : File_Type; Item : out String; Count : out Natural);
    pragma Import (C, Read_Line, "__ada_text_get_line");
    procedure Get_Line (File : in File_Type; Item : out String; Last : out Natural) is
        count : Natural;
    begin
        Read_Line (File, Item, count);
        Last := (Item'First - 1) + count;
    end Get_Line;
    procedure Get_Line (Item : out String; Last : out Natural) is
    begin
        Get_Line (Current_Input, Item, Last);
    end Get_Line;

    package body Integer_IO is


        Digit_Set : constant String (1 .. 16) := "0123456789ABCDEF";

        -- Room for the widest thing Put can produce: a sign, a base written as two
        -- digits, the two marks around the number, and the number itself in base
        -- two.
        Text_Length : constant := 72;

        -- Item spelled out in the given base, right justified in Text and without
        -- any padding.  First says where it starts.  Ada writes a number in a base
        -- other than ten as 16#FF#, with the sign in front of the whole thing.
        procedure Image_Of (Item  : in Num;
                            Base  : in Number_Base;
                            Text  : out String;
                            First : out Natural) is
            Left     : Long_Integer := Long_Integer (Item);
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

            -- The remainder is taken from whichever side Left is on and its sign
            -- dropped afterwards, since the most negative value of a type has no
            -- positive counterpart to negate it into.
            while Left /= 0 loop
                Digit := Integer (abs (Left rem Long_Integer (Base)));
                Left := Left / Long_Integer (Base);
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

            if Item < 0 then
                Position := Position - 1;
                Text (Position) := '-';
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
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (field);
                end;
            end;
        end Put;

        procedure Put (File  : in File_Type;
                       Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base) is
            Text  : String (1 .. Text_Length);
            First : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (File, field);
                end;
            end;
        end Put;

        -- A number the instantiated type cannot hold is a Data_Error and not a
        -- Constraint_Error, which is why the range is tested here rather than left
        -- to the assignment below it.
        procedure Store (Word : in String; Item : out Num) is
            Value : Long_Integer;
        begin
            Value := Long_Integer'Value (Word);
            if Value < Long_Integer (Num'First) or else Value > Long_Integer (Num'Last) then
                raise Data_Error;
            end if;
            Item := Num (Value);
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Store;

        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Normalized (Scanning.Token (Width, 0)), Item);
        end Get;
        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Normalized (Scanning.Token (File, Width, 0)), Item);
        end Get;
        procedure Get (From : String; Item : out Num; Last : out Positive) is
            count : Natural := Scanning.Token_Length (From, 0);
        begin
            Store (Scanning.Normalized (From (From'First .. (From'First - 1) + count)), Item);
            Last := (From'First - 1) + count;
        end Get;
        procedure Put (To : out String; Item : Num; Base : Number_Base := Default_Base) is
            text : String (1 .. Text_Length);
            first : Natural;
            length : Natural;
        begin
            Image_Of (Item, Base, text, first);
            length := Text_Length - first + 1;
            if length > To'Length then
                raise Layout_Error;
            end if;
            To := (others => ' ');
            To (To'Last - (length - 1) .. To'Last) := text (first .. Text_Length);
        end Put;
    end Integer_IO;

    package body Modular_IO is


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
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (field);
                end;
            end;
        end Put;

        procedure Put (File  : in File_Type;
                       Item  : in Num;
                       Width : in Field := Default_Width;
                       Base  : in Number_Base := Default_Base) is
            Text  : String (1 .. Text_Length);
            First : Natural;
        begin
            Image_Of (Item, Base, Text, First);
            declare
                length : Natural := Text_Length - First + 1;
            begin
                if Width > length then length := Width; end if;
                declare
                    field : String (1 .. length) := (others => ' ');
                begin
                    field (length - (Text_Length - First) .. length) := Text (First .. Text_Length);
                    Scanning.Put_Field (File, field);
                end;
            end;
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
            Store (From (From'First .. (From'First - 1) + Count), Item);
            Last := (From'First - 1) + Count;
        end Get;

    end Modular_IO;

    package body Float_IO is


        -- Laying a real number out is arithmetic on the digits of its
        -- representation, which is the one part of this package the C run time
        -- still does; everything about the type is decided here.
        procedure Write (Item : in Long_Float; Fore : in Field; Aft : in Field; Exp : in Field);
        pragma Import (C, Write, "__ada_put_float");

        procedure Write (File : in File_Type;
                         Item : in Long_Float;
                         Fore : in Field;
                         Aft  : in Field;
                         Exp  : in Field);
        pragma Import (C, Write, "__ada_text_put_float");

        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft  : in Field := Default_Aft;
                       Exp  : in Field := Default_Exp) is
        begin
            Write (Long_Float (Item), Fore, Aft, Exp);
        end Put;

        procedure Put (File : in File_Type;
                       Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft  : in Field := Default_Aft;
                       Exp  : in Field := Default_Exp) is
        begin
            Write (File, Long_Float (Item), Fore, Aft, Exp);
        end Put;

        procedure Store (Text : String; Item : out Num) is
        begin
            Item := Num'Value (Scanning.Normalized (Text));
        exception
            when Constraint_Error => raise Data_Error;
        end Store;
        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Token (Width, 1), Item);
        end Get;
        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Store (Scanning.Token (File, Width, 1), Item);
        end Get;
        procedure Get (From : String; Item : out Num; Last : out Positive) is
            count : Natural := Scanning.Token_Length (From, 1);
        begin
            Store (From (From'First .. (From'First - 1) + count), Item);
            Last := (From'First - 1) + count;
        end Get;
        procedure Write (To : out String; Item : Long_Float; Aft, Exp : Field);
        pragma Import (C, Write, "__ada_text_float_string");
        procedure Put (To : out String; Item : Num;
                       Aft : Field := Default_Aft; Exp : Field := Default_Exp) is
        begin
            Write (To, Long_Float (Item), Aft, Exp);
        end Put;
    end Float_IO;

    package body Fixed_IO is

        -- Imported Num parameters and results carry the scaled integer directly.
        -- Small is an exact power of two, so only its exponent crosses as a float.
        function Scale (Small : Long_Float) return Integer;
        pragma Import (C, Scale, "__ada_fixed_scale");
        Bits : constant Integer := Scale (Long_Float (Num'Small));

        procedure Write (File : File_Type; Item : Num; Bits, Fore, Aft, Exp : Integer);
        pragma Import (C, Write, "__ada_fixed_put");
        procedure Write (To : out String; Item : Num; Bits, Aft, Exp : Integer);
        pragma Import (C, Write, "__ada_fixed_put_string");
        function Read (File : File_Type; Bits : Integer; Low, High : Num; Width : Integer) return Num;
        pragma Import (C, Read, "__ada_fixed_get");
        function Read (From : String; Bits : Integer; Low, High : Num; Last : out Natural) return Num;
        pragma Import (C, Read, "__ada_fixed_get_string");

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Item := Read (File, Bits, Num'First, Num'Last, Width);
        end Get;
        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Get (Current_Input, Item, Width);
        end Get;
        procedure Get (From : in String; Item : out Num; Last : out Positive) is
            Count : Natural;
        begin
            Item := Read (From, Bits, Num'First, Num'Last, Count);
            Last := (From'First - 1) + Count;
        end Get;

        procedure Put (File : in File_Type; Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (File, Item, Bits, Fore, Aft, Exp);
        end Put;
        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Put (Current_Output, Item, Fore, Aft, Exp);
        end Put;
        procedure Put (To : out String; Item : in Num;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (To, Item, Bits, Aft, Exp);
        end Put;
    end Fixed_IO;

    package body Decimal_IO is

        -- Tagged decimal scale, matching the shared fixed-point runtime ABI.
        Bits : constant Integer := 100 + Num'Scale;

        procedure Write (File : File_Type; Item : Num; Bits, Fore, Aft, Exp : Integer);
        pragma Import (C, Write, "__ada_fixed_put");
        procedure Write (To : out String; Item : Num; Bits, Aft, Exp : Integer);
        pragma Import (C, Write, "__ada_fixed_put_string");
        function Read (File : File_Type; Bits : Integer; Low, High : Num; Width : Integer) return Num;
        pragma Import (C, Read, "__ada_fixed_get");
        function Read (From : String; Bits : Integer; Low, High : Num; Last : out Natural) return Num;
        pragma Import (C, Read, "__ada_fixed_get_string");

        procedure Get (File : in File_Type; Item : out Num; Width : in Field := 0) is
        begin
            Item := Read (File, Bits, Num'First, Num'Last, Width);
        end Get;
        procedure Get (Item : out Num; Width : in Field := 0) is
        begin
            Get (Current_Input, Item, Width);
        end Get;
        procedure Get (From : in String; Item : out Num; Last : out Positive) is
            Count : Natural;
        begin
            Item := Read (From, Bits, Num'First, Num'Last, Count);
            Last := (From'First - 1) + Count;
        end Get;

        procedure Put (File : in File_Type; Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (File, Item, Bits, Fore, Aft, Exp);
        end Put;
        procedure Put (Item : in Num;
                       Fore : in Field := Default_Fore;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Put (Current_Output, Item, Fore, Aft, Exp);
        end Put;
        procedure Put (To : out String; Item : in Num;
                       Aft : in Field := Default_Aft;
                       Exp : in Field := Default_Exp) is
        begin
            Write (To, Item, Bits, Aft, Exp);
        end Put;
    end Decimal_IO;

    package body Enumeration_IO is
        procedure Format (Item : Enum; Width : Field; Set : Type_Set; File : File_Type) is
            text : String := Enum'Image (Item);
            length : Natural := text'Length;
        begin
            if Width > length then length := Width; end if;
            for i in text'Range loop
                if Set = Lower_Case and text (text'First) /= Character'Val (39)
                    and text (i) >= 'A' and text (i) <= 'Z' then
                    text (i) := Character'Val (Character'Pos (text (i)) + 32);
                end if;
            end loop;
            declare
                field : String (1 .. length) := (others => ' ');
            begin
                field (1 .. text'Length) := text;
                Scanning.Put_Field (File, field);
            end;
        end Format;
        procedure Put (Item : Enum; Width : Field := Default_Width; Set : Type_Set := Default_Setting) is
        begin
            Format (Item, Width, Set, Current_Output);
        end Put;
        procedure Put (File : File_Type; Item : Enum;
                       Width : Field := Default_Width; Set : Type_Set := Default_Setting) is
        begin
            Format (Item, Width, Set, File);
        end Put;

        -- A word that names no literal of the type is a Data_Error here, though
        -- 'Value reports it as the Constraint_Error it is in every other setting.
        procedure Get (Item : out Enum) is

        begin
            Item := Enum'Value (Scanning.Token (0, 2));
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Get;

        procedure Get (File : in File_Type; Item : out Enum) is

        begin
            Item := Enum'Value (Scanning.Token (File, 0, 2));
        exception
            when Constraint_Error =>
                raise Data_Error;
        end Get;

        procedure Get (From : String; Item : out Enum; Last : out Positive) is
            count : Natural := Scanning.Token_Length (From, 2);
        begin
            Item := Enum'Value (From (From'First .. (From'First - 1) + count));
            Last := (From'First - 1) + count;
        exception
            when Constraint_Error => raise Data_Error;
        end Get;
        procedure Put (To : out String; Item : Enum; Set : Type_Set := Default_Setting) is
            text : String := Enum'Image (Item);
        begin
            if text'Length > To'Length then
                raise Layout_Error;
            end if;
            To := (others => ' ');
            for i in text'Range loop
                if Set = Lower_Case and text (text'First) /= Character'Val (39) and text (i) >= 'A' and text (i) <= 'Z' then
                    text (i) := Character'Val (Character'Pos (text (i)) + 32);
                end if;
                To (To'First + (i - text'First)) := text (i);
            end loop;
        end Put;
    end Enumeration_IO;
end Ada.Text_IO;
