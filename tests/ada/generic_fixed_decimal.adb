procedure generic_fixed_decimal is
    generic
        type Item is delta <> digits <>;
    package P is
    end P;
begin
    null;
end generic_fixed_decimal;
