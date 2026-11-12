with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
with Ada.Streams.Stream_IO;
with Ada.Storage_IO;
with Packed_Signed_Model; use Packed_Signed_Model;
procedure PackedSigned is
    type Medium is range -16 .. 15;
    type Positive_Small is range 0 .. 31;
    type Positive_Array is array (1 .. 9) of Positive_Small;
    pragma Pack (Positive_Array);
    type Sized is range -7 .. 7;
    for Sized'Size use 16;
    subtype Sized_View is Sized range -3 .. 3;
    type Sized_Array is array (1 .. 3) of Sized;
    pragma Pack (Sized_Array);
    type Sized_View_Array is array (1 .. 3) of Sized_View;
    pragma Pack (Sized_View_Array);
    type Tiny is range -1 .. 0;
    type Wide is range -9223372036854775807 .. 9223372036854775807;
    type Wide_Array is array (1 .. 3) of Wide;
    pragma Pack (Wide_Array);
    type Tiny_Array is array (1 .. 11) of Tiny;
    pragma Pack (Tiny_Array);
    type Values_Access is access Values;
    procedure Free is new Ada.Unchecked_Deallocation (Values, Values_Access);
    procedure Check (n : Positive) is
        type Dynamic is array (1 .. n) of Medium;
        pragma Pack (Dynamic);
        type Same_Layout is array (Integer range <>) of Small;
        pragma Pack (Same_Layout);
        type Rows is array (1 .. 3) of Dynamic;
        matrix : Rows := (others => (others => -16));
        a : Values (-3 .. n) := (others => -7);
        b : Values := Copy (a);
        pointer : Values_Access := new Values'(a);
        converted : Same_Layout := Same_Layout (a);
        empty : Values (4 .. 3);
        nullCopy : Values := Copy (empty);
        object : Holder;
        alias : Small renames a (-3);
        sizedValues : Sized_Array := (-7, 0, 7);
        sizedView : Sized_View_Array := (-3, 0, 3);
        positiveValues : Positive_Array := (others => 31);
        tinyValues : Tiny_Array := (others => -1);
        wideValues : Wide_Array := (Wide'First, 0, Wide'Last);
        procedure Set (value : out Small) is
        begin
            value := -5;
        end Set;
        procedure Fail (value : in out Small) is
        begin
            value := 6;
            raise Constraint_Error;
        end Fail;
    begin
        if a'Component_Size /= 4 or a (-3)'Size /= 4 or alias'Size /= 4
            or Dynamic'Size /= n * 5 or matrix (1)'Size /= ((n * 5 + 7) / 8) * 8
            or Tiny_Array'Size /= 11 or wideValues'Component_Size /= 64
            or nullCopy'Length /= 0
        then
            raise Program_Error;
        end if;
        for i in a'Range loop
            a (i) := Small ((i + 3) mod 15 - 7);
        end loop;
        b := Copy (a);
        converted := Same_Layout (a);
        for i in a'Range loop
            if converted (i) /= a (i) then raise Program_Error; end if;
        end loop;
        Negate (b);
        for i in a'Range loop
            if b (i) /= -a (i) then raise Program_Error; end if;
        end loop;
        if pointer.all (-3) /= -7 or pointer.all'Length /= a'Length then
            raise Program_Error;
        end if;
        pointer.all (-2) := 5;
        if pointer.all (-3) /= -7 or pointer.all (-1) /= -7 then
            raise Program_Error;
        end if;
        Free (pointer);
        if pointer /= null then raise Program_Error; end if;
        Set (a (-2));
        begin
            Fail (a (-2));
        exception
            when Constraint_Error => null;
        end;
        if a (-2) /= -5 or a (-3) /= -7 then raise Program_Error; end if;
        for i in 1 .. n loop
            matrix (2)(i) := Medium (i mod 32 - 16);
        end loop;
        for i in 1 .. n loop
            if matrix (2)(i) /= Medium (i mod 32 - 16)
                or matrix (1)(i) /= -16 or matrix (3)(i) /= -16
            then
                raise Program_Error;
            end if;
        end loop;
        if sizedValues'Component_Size /= 16 or sizedView'Component_Size /= 16
            or Sized_Array'Size /= 48 or Sized_View_Array'Size /= 48
            or sizedValues (1) /= -7 or sizedView (3) /= 3
        then
            raise Program_Error;
        end if;
        sizedView (2) := -3;
        if sizedView (1) /= -3 or sizedView (2) /= -3 or sizedView (3) /= 3 then
            raise Program_Error;
        end if;
        positiveValues (4) := 17;
        if positiveValues'Component_Size /= 5 or positiveValues (4) /= 17
            or positiveValues (3) /= 31 or positiveValues (5) /= 31
        then
            raise Program_Error;
        end if;
        tinyValues (6) := 0;
        for i in tinyValues'Range loop
            if (i = 6 and tinyValues (i) /= 0)
                or (i /= 6 and tinyValues (i) /= -1)
            then
                raise Program_Error;
            end if;
        end loop;
        if wideValues (1) /= Wide'First or wideValues (3) /= Wide'Last then
            raise Program_Error;
        end if;
        wideValues (2) := Wide'First;
        if wideValues (2) /= Wide'First then raise Program_Error; end if;
        object.items (0) := -7;
        if object.items (-1) /= -3 or object.items (1) /= -3 then
            raise Program_Error;
        end if;
    end Check;
    task type Worker is
        entry Done;
    end Worker;
    task body Worker is
    begin
        for i in 1 .. 4 loop
            Check (i * 7);
        end loop;
        accept Done;
    end Worker;
begin
    declare
        workers : array (1 .. 4) of Worker;
    begin
        for i in workers'Range loop
            workers (i).Done;
        end loop;
    end;
    declare
        file : Ada.Streams.Stream_IO.File_Type;
        channel : Ada.Streams.Stream_IO.Stream_Access;
        type Medium_Array is array (1 .. 3) of Medium;
        pragma Pack (Medium_Array);
        cross : Medium_Array := (-16, -1, 15);
        crossCopy : Medium_Array := (others => 0);
        package Medium_Native is new Ada.Storage_IO (Medium_Array);
        mediumBuffer : Medium_Native.Buffer_Type;
        subtype Triple is Values (1 .. 3);
        package Native is new Ada.Storage_IO (Triple);
        buffer : Native.Buffer_Type;
        failed : Boolean := False;
        a : Values (1 .. 3) := (-7, 0, 7);
        b : Values (1 .. 3) := (others => 1);
    begin
        Ada.Streams.Stream_IO.Create (file, Ada.Streams.Stream_IO.Out_File, "packedsigned.tmp");
        channel := Ada.Streams.Stream_IO.Stream (file);
        Values'Write (channel, a);
        Medium_Array'Write (channel, cross);
        Ada.Streams.Stream_IO.Close (file);
        Ada.Streams.Stream_IO.Open (file, Ada.Streams.Stream_IO.In_File, "packedsigned.tmp");
        channel := Ada.Streams.Stream_IO.Stream (file);
        Values'Read (channel, b);
        Medium_Array'Read (channel, crossCopy);
        Ada.Streams.Stream_IO.Delete (file);
        if a /= b or cross /= crossCopy then raise Program_Error; end if;
        Native.Write (buffer, a);
        if Integer (Native.Buffer_Size) /= 2 or Integer (buffer (1)) /= 9
            or Integer (buffer (2)) /= 7
        then
            raise Program_Error;
        end if;
        b := (others => 1);
        Native.Read (buffer, b);
        if a /= b then raise Program_Error; end if;
        buffer (1) := 8;
        begin
            Native.Read (buffer, b);
        exception
            when Native.Data_Error => failed := True;
        end;
        if not failed or a /= b then raise Program_Error; end if;
        Medium_Native.Write (mediumBuffer, cross);
        crossCopy := (others => 0);
        Medium_Native.Read (mediumBuffer, crossCopy);
        if cross /= crossCopy then raise Program_Error; end if;
    end;
    Ada.Text_IO.Put_Line ("packed signed arrays OK");
end PackedSigned;
