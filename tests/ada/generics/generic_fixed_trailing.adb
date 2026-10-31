procedure generic_fixed_trailing is
    generic
        type Item is delta <> range -1.0 .. 1.0;
    package P is
    end P;
begin
    null;
end generic_fixed_trailing;
