setClass("testclass", representation(aa="character"))

setMethod("initialize", "testclass", function(.Object, aa) {
    print ("initialize called")
    .Object
})

setMethod(".finalize", "testclass", function(.Object) {
    print (".finalize called")
})

setMethod("finalize", "testclass", function(.Object) {
    print ("finalize called")
})

tc = new ("testclass", aa="this is aa")
tc


f <- function(e) print("cleaning....")
g <- function(x){ e <- environment(); reg.finalizer(e, f) }
g()
gc()
