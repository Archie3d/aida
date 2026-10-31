with GenericDerivedModel;
generic
    type T is new GenericDerivedModel.Root;
package GenericDerivedLibrary is
    function Read_Ancestor (X : T) return Integer;
end GenericDerivedLibrary;
