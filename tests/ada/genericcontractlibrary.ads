with GenericClasswideModel;
generic
    type T (<>) is new GenericClasswideModel.Root with private;
package GenericContractLibrary is
    function Score (Object : T'Class) return Integer;
    generic
        with function Measure (Object : T) return Integer is abstract Read;
    function Nested_Score (Object : T'Class) return Integer;
end GenericContractLibrary;
