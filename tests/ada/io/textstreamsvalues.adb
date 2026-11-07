with Ada.Text_IO;
with Ada.Text_IO.Text_Streams;
with Ada.Wide_Text_IO;
with Ada.Wide_Text_IO.Text_Streams;
with Ada.Wide_Wide_Text_IO;
with Ada.Wide_Wide_Text_IO.Text_Streams;
with Ada.Strings.Unbounded;
with Ada.Strings.Wide_Unbounded;
with Ada.Strings.Wide_Wide_Unbounded;
procedure TextStreamsValues is
    procedure Check_1 is
        package T renames Ada.Text_IO;
        package S renames Ada.Text_IO.Text_Streams;
        package U renames Ada.Strings.Unbounded;
        F : T.File_Type;
        P : S.Stream_Access;
        type Many is array (1 .. 3) of U.Unbounded_String;
        A : Many := (U.To_Unbounded_String ("cat"), U.To_Unbounded_String ("testing"), U.To_Unbounded_String ("stream"));
        B, C : Many;
    begin
        T.Create (F);
        P := S.Stream (F);
        for I in A'Range loop U.Unbounded_String'Write (P, A (I)); end loop;
        Many'Output (P, A);
        T.Reset (F, T.In_File);
        for I in A'Range loop U.Unbounded_String'Read (P, B (I)); end loop;
        C := Many'Input (P);
        if A /= B or A /= C then raise Program_Error; end if;
        for I in A'Range loop
            if U.To_String (A (I)) /= U.To_String (B (I))
                or U.To_String (A (I)) /= U.To_String (C (I)) then
                raise Program_Error;
            end if;
        end loop;
        T.Close (F);
    end Check_1;
    procedure Check_2 is
        package T renames Ada.Wide_Text_IO;
        package S renames Ada.Wide_Text_IO.Text_Streams;
        package U renames Ada.Strings.Wide_Unbounded;
        F : T.File_Type;
        P : S.Stream_Access;
        type Many is array (1 .. 3) of U.Unbounded_Wide_String;
        A : Many := (U.To_Unbounded_Wide_String ("cat"), U.To_Unbounded_Wide_String ("testing"), U.To_Unbounded_Wide_String ("stream"));
        B, C : Many;
    begin
        T.Create (F);
        P := S.Stream (F);
        for I in A'Range loop U.Unbounded_Wide_String'Write (P, A (I)); end loop;
        Many'Output (P, A);
        T.Reset (F, T.In_File);
        for I in A'Range loop U.Unbounded_Wide_String'Read (P, B (I)); end loop;
        C := Many'Input (P);
        if A /= B or A /= C then raise Program_Error; end if;
        for I in A'Range loop
            if U.To_Wide_String (A (I)) /= U.To_Wide_String (B (I))
                or U.To_Wide_String (A (I)) /= U.To_Wide_String (C (I)) then
                raise Program_Error;
            end if;
        end loop;
        T.Close (F);
    end Check_2;
    procedure Check_3 is
        package T renames Ada.Wide_Wide_Text_IO;
        package S renames Ada.Wide_Wide_Text_IO.Text_Streams;
        package U renames Ada.Strings.Wide_Wide_Unbounded;
        F : T.File_Type;
        P : S.Stream_Access;
        type Many is array (1 .. 3) of U.Unbounded_Wide_Wide_String;
        A : Many := (U.To_Unbounded_Wide_Wide_String ("cat"), U.To_Unbounded_Wide_Wide_String ("testing"), U.To_Unbounded_Wide_Wide_String ("stream"));
        B, C : Many;
    begin
        T.Create (F);
        P := S.Stream (F);
        for I in A'Range loop U.Unbounded_Wide_Wide_String'Write (P, A (I)); end loop;
        Many'Output (P, A);
        T.Reset (F, T.In_File);
        for I in A'Range loop U.Unbounded_Wide_Wide_String'Read (P, B (I)); end loop;
        C := Many'Input (P);
        if A /= B or A /= C then raise Program_Error; end if;
        for I in A'Range loop
            if U.To_Wide_Wide_String (A (I)) /= U.To_Wide_Wide_String (B (I))
                or U.To_Wide_Wide_String (A (I)) /= U.To_Wide_Wide_String (C (I)) then
                raise Program_Error;
            end if;
        end loop;
        T.Close (F);
    end Check_3;
begin
    Check_1;
    Check_2;
    Check_3;
    Ada.Text_IO.Put_Line ("text stream values passed");
end TextStreamsValues;
