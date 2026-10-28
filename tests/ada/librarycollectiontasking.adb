with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
with Ada.Text_IO;
procedure LibraryCollectionTasking is
    package Texts is new Ada.Containers.Vectors (Positive, Unbounded_String);
    protected Results is
        procedure Done (Good : Boolean);
        function Passed return Boolean;
    private
        Completed : Natural := 0;
        Errors : Natural := 0;
    end Results;
    protected body Results is
        procedure Done (Good : Boolean) is
        begin
            Completed := Completed + 1;
            if not Good then
                Errors := Errors + 1;
            end if;
        end Done;
        function Passed return Boolean is
        begin
            return Completed = 4 and Errors = 0;
        end Passed;
    end Results;
    -- Both access-type collections belong to the enclosing master and are
    -- shared by all workers, including allocation and explicit deallocation.
    procedure Exercise is
        task type Worker;
        task body Worker is
            A, B : Texts.Vector;
            Text : Unbounded_String;
        begin
            for Round in 1 .. 20 loop
                Set_Unbounded_String (Text, "shared collection");
                for I in 1 .. 12 loop
                    Texts.Append (A, Text);
                    Append (Text, '!');
                end loop;
                B := A;
                Texts.Clear (A);
                if Texts.Length (B) /= 12 or else Texts.Element (B, 1) /= "shared collection" then
                    raise Program_Error;
                end if;
                Texts.Delete_Last (B, 12);
            end loop;
            Results.Done (True);
        exception
            when others => Results.Done (False);
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise;
begin
    Exercise;
    if not Results.Passed then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("library collections tasking passed");
end LibraryCollectionTasking;
