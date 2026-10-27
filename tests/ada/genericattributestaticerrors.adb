procedure GenericAttributeStaticErrors is
    generic
        type T is (<>);
    package Views is
        First : constant Integer := T'Pos (T'First);
    end Views;
    package body Views is
        procedure Check (Object : Integer) is
        begin
            case Object is
                when T'Pos (T'First) => null;
                when others => null;
            end case;
        end Check;
    end Views;
begin
    null;
end GenericAttributeStaticErrors;
