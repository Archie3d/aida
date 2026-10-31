with widewideStringMappingSupport;
with Ada.Strings.Wide_Wide_Maps;
with Ada.Strings.Wide_Wide_Maps.Wide_Wide_Constants;
with Ada.Strings.Wide_Wide_Fixed;
with Ada.Strings.Wide_Wide_Bounded;
with Ada.Strings.Wide_Wide_Unbounded;
with Ada.Wide_Wide_Text_IO;
procedure widewideStringMappingTasking is
    package B is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (16);
    package U renames Ada.Strings.Wide_Wide_Unbounded;
    use type Ada.Strings.Wide_Wide_Maps.Wide_Wide_Character_Set;
    use type B.Bounded_Wide_Wide_String;
    use type U.Unbounded_Wide_Wide_String;
    mapping : constant Ada.Strings.Wide_Wide_Maps.Wide_Wide_Character_Mapping_Function :=
        widewideStringMappingSupport.Task_Lower'Access;
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
        bounded : B.Bounded_Wide_Wide_String;
        unbounded : U.Unbounded_Wide_Wide_String;
    begin
        for i in 1 .. 100 loop
            bounded := B.To_Bounded_Wide_Wide_String ("AbAB");
            unbounded := U.To_Unbounded_Wide_Wide_String ("AbAB");
            B.Translate (bounded, mapping);
            U.Translate (unbounded, mapping);
            if bounded /= "abab" or unbounded /= "abab" or
                Ada.Strings.Wide_Wide_Fixed.Count ("ABAB", "ab", mapping) /= 2 or
                Ada.Strings.Wide_Wide_Maps.Value
                    (Ada.Strings.Wide_Wide_Maps.Wide_Wide_Constants.Lower_Case_Map, 'A') /= 'a' then
                raise Program_Error;
            end if;
        end loop;
        declare
            package M renames Ada.Strings.Wide_Wide_Maps;
            Top : constant Wide_Wide_Character := Wide_Wide_Character'Last;
            Set : M.Wide_Wide_Character_Set := M.To_Set (M.Wide_Wide_Character_Range'(Wide_Wide_Character'Val (256), Top));
            Copy : M.Wide_Wide_Character_Set := Set;
            Mapping : M.Wide_Wide_Character_Mapping := M.To_Mapping (Wide_Wide_String'('Ω', Top), Wide_Wide_String'(Top, 'Ж'));
        begin
            Set := not Set;
            if M.Is_In (Top, Set) or not M.Is_In (Top, Copy) or M.Value (Mapping, 'Ω') /= Top then
                raise Program_Error;
            end if;
        end;
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
    Ada.Wide_Wide_Text_IO.Put_Line ("widewideStringMappingTasking: passed");
end widewideStringMappingTasking;
