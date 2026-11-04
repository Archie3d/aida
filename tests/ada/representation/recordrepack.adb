with Ada.Text_IO; use Ada.Text_IO;
with Ada.Streams.Stream_IO;
with Repack_Operations; use Repack_Operations;
procedure RecordRepack is
    type Partial is new Parent;
    for Partial use record
        Right at 0 range 0 .. 31;
    end record;
    type Items is array (1 .. 2) of Swapped;

    procedure Check is
        P : Parent := Make (2, 3);
        S : Swapped := Swapped (P);
        U : Unaligned := Make (4, 5);
        T : Partial := Partial (S);
        Row : Items := (S, S);
        Counter : Natural := 0;
        function Index return Integer is
        begin
            Counter := Counter + 1;
            return 2;
        end Index;
        C : Compact;
        Fixed : Compact (False);
        Active : Compact (True);
        Narrowed : Byte_Count;
        Boxed : Compact := (Flag => True, Y => <>);
        Rejected : Boolean := False;
    begin
        if Parent'Size /= Swapped'Size or else S.Left'Position /= 4 or else
           S.Right'Position /= 0 or else T.Left'Position /= 4 or else
           U.Right'Position /= 5 or else Parent (S) /= P or else
           Measure (S) /= 203 or else Measure (U) /= 405 or else
           Measure (T) /= 203 then
            raise Program_Error;
        end if;
        Update (Row (Index));
        if Counter /= 1 or else Measure (Row (1)) /= 203 or else Measure (Row (2)) /= 303 then
            raise Program_Error;
        end if;
        Initialize (U);
        if Measure (U) /= 1112 then
            raise Program_Error;
        end if;
        begin
            Fail (U);
        exception
            when Constraint_Error => Rejected := True;
        end;
        if not Rejected or else Measure (U) /= 1112 then
            raise Program_Error;
        end if;
        if Active.Y /= 8 or else Boxed.Y /= 8 or else Narrowed.Value /= 3 then
            raise Program_Error;
        end if;
        Switch (C);
        if not C.Flag or else C.Y /= -3 or else Choice (C).Y /= -3 then
            raise Program_Error;
        end if;
        Rejected := False;
        begin
            Switch (Fixed);
        exception
            when Constraint_Error => Rejected := True;
        end;
        if not Rejected or else Fixed.Flag or else Fixed.X /= -4 then
            raise Program_Error;
        end if;
    end Check;
begin
    Check;
    declare
        package Files renames Ada.Streams.Stream_IO;
        File : Files.File_Type;
        Item : Swapped;
    begin
        Files.Create (File, Files.Out_File, "recordrepack.tmp");
        Integer'Output (Files.Stream (File), 17);
        Integer'Output (Files.Stream (File), 18);
        Integer'Output (Files.Stream (File), 13);
        Integer'Output (Files.Stream (File), 14);
        Files.Close (File);
        Files.Open (File, Files.In_File, "recordrepack.tmp");
        Item := Swapped'Input (Files.Stream (File));
        if Measure (Item) /= 1718 then
            raise Program_Error;
        end if;
        Swapped'Read (Files.Stream (File), Item);
        if Measure (Item) /= 1314 then
            raise Program_Error;
        end if;
        Files.Delete (File);
    end;
    declare
        task type Worker;
        task body Worker is
        begin
            Check;
            Put_Line ("repacked record OK");
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
end RecordRepack;
