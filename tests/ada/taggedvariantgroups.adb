with Ada.Text_IO;
procedure TaggedVariantGroups is
    procedure Run is
        type Root (First : Integer) is tagged record
            case First is
                when 0 => A : Character := 'A';
                when others => B : Character := 'B';
            end case;
        end record;
        type Child (Parent, Second : Integer) is new Root (Parent) with record
            case Second is
                when 1 => C : Character := 'C';
                when others => D : Character := 'D';
            end case;
        end record;
        type Grandchild (One, Two, Third : Integer) is new Child (One, Two) with record
            case Third is
                when 0 => E : Character := 'E';
                when others => F : Character := 'F';
            end case;
        end record;
        procedure Check (Item : Grandchild) is
        begin
            if Item.One = 0 then
                if Item.A /= 'A' then raise Program_Error; end if;
            else
                if Item.B /= 'B' then raise Program_Error; end if;
            end if;
            if Item.Two = 1 then
                if Item.C /= 'C' then raise Program_Error; end if;
            else
                if Item.D /= 'D' then raise Program_Error; end if;
            end if;
            if Item.Third = 0 then
                if Item.E /= 'E' then raise Program_Error; end if;
            else
                if Item.F /= 'F' then raise Program_Error; end if;
            end if;
        end Check;
        Static_One : Grandchild (0, 0, 0);
        Static_Two : Grandchild (1, 1, 1);
        Static_Three : Grandchild (0, 1, 1);
        Static_Four : Grandchild (1, 0, 0);
        type Link is access Grandchild;
        Allocated : Link := new Grandchild (0, 0, 1);
    begin
        if Static_Three.A /= 'A' or else Static_Three.C /= 'C' or else Static_Three.F /= 'F'
            or else Static_Four.B /= 'B' or else Static_Four.D /= 'D' or else Static_Four.E /= 'E' then
            raise Program_Error;
        end if;
        Check (Static_One);
        Check (Static_Two);
        Check (Static_Three);
        Check (Static_Four);
        Check (Allocated.all);
        for I in 0 .. 2 loop
            for J in 0 .. 2 loop
                for K in 0 .. 2 loop
                    declare
                        Dynamic : Grandchild (I, J, K);
                    begin
                        Check (Dynamic);
                    end;
                end loop;
            end loop;
        end loop;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("tagged variant groups worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tagged variant groups passed");
end TaggedVariantGroups;
