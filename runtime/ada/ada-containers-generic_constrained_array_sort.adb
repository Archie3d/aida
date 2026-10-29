-- Ada 2005 container implementation; ownership and cursor machinery is shared.
procedure Ada.Containers.Generic_Constrained_Array_Sort (Container : in out Array_Type) is
    Size : Long_Integer := Container'Length;
    function Index (Offset : Long_Integer) return Index_Type is
    begin
        return Index_Type'Val (Index_Type'Pos (Container'First) + (Offset - 1));
    end Index;
    procedure Swap (I, J : Long_Integer) is
        Temp : Element_Type := Container (Index (I));
    begin
        Container (Index (I)) := Container (Index (J));
        Container (Index (J)) := Temp;
    end Swap;
    procedure Sift (Start, Finish : Long_Integer) is
        Root : Long_Integer := Start;
        Child : Long_Integer;
    begin
        while Root <= Finish / 2 loop
            Child := Root * 2;
            if Child < Finish and then Container (Index (Child)) < Container (Index (Child + 1)) then
                Child := Child + 1;
            end if;
            exit when not (Container (Index (Root)) < Container (Index (Child)));
            Swap (Root, Child);
            Root := Child;
        end loop;
    end Sift;
begin
    for I in reverse 1 .. Size / 2 loop
        Sift (I, Size);
    end loop;
    for I in reverse 2 .. Size loop
        Swap (1, I);
        Sift (1, I - 1);
    end loop;
end Ada.Containers.Generic_Constrained_Array_Sort;
