with Ada.Characters.Handling;
with Ada.Strings.Maps;
with Ada.Strings.Maps.Constants;
with Ada.Strings.Fixed;
with Ada.Strings.Bounded;
with Ada.Strings.Unbounded;
with Ada.Text_IO;
procedure StringMappingTasking is
    package B is new Ada.Strings.Bounded.Generic_Bounded_Length (16);
    package U renames Ada.Strings.Unbounded;
    use type B.Bounded_String;
    use type U.Unbounded_String;
    mapping : constant Ada.Strings.Maps.Character_Mapping_Function :=
        Ada.Characters.Handling.To_Lower'Access;
    protected completion is
        procedure done;
        function count return Natural;
    private
        finished : Natural := 0;
    end completion;
    protected body completion is
        procedure done is
        begin
            finished := finished + 1;
        end done;
        function count return Natural is
        begin
            return finished;
        end count;
    end completion;
    task type Worker;
    task body Worker is
        bounded : B.Bounded_String;
        unbounded : U.Unbounded_String;
    begin
        for i in 1 .. 100 loop
            bounded := B.To_Bounded_String ("AbAB");
            unbounded := U.To_Unbounded_String ("AbAB");
            B.Translate (bounded, mapping);
            U.Translate (unbounded, mapping);
            if bounded /= "abab" or unbounded /= "abab" or
                Ada.Strings.Fixed.Count ("ABAB", "ab", mapping) /= 2 or
                Ada.Strings.Maps.Value
                    (Ada.Strings.Maps.Constants.Lower_Case_Map, 'A') /= 'a' then
                raise Program_Error;
            end if;
        end loop;
        completion.done;
    end Worker;
begin
    declare
        workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    if completion.count /= 4 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("stringmappingtasking: passed");
end StringMappingTasking;
