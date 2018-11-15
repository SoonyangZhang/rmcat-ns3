import os, sys

discardedFrames = 25

sequences = ['Foreman', 'News', 'Suzie']

inDirs = [x + '_lookahead_1' for x in sequences]
prefixes = [[x, 'ProRes'] for x in sequences]

outDir = 'Concat'
outPrefixes = [outDir, 'ProRes']

for d in inDirs + [outDir]:
    if not os.path.isdir(d):
        sys.stderr.write('Fatal: You need to run this script no top of the three dirs to stitch')
        os.exit(1)

for filename in os.listdir(inDirs[0]):
    tokens = filename.split("_")
    nprefixes = len(prefixes[0])
    for i in range(1, len(prefixes)):
        assert(len(prefixes[i]) == nprefixes)

    for i in range(nprefixes):
        assert(tokens[i] == prefixes[0][i])

    suffix = "_".join(tokens[nprefixes:])

    outName = os.path.join(outDir, "_".join(outPrefixes + [suffix]))
    assert(not os.path.exists(outName))
    print("Writing file " + outName)

    with open(outName, "w") as outF:
        for i in range(len(inDirs)):
            inName = os.path.join(inDirs[i], "_".join(prefixes[i] + [suffix]))
            assert(os.path.isfile(inName))
            with open(inName) as inF:
                lines = inF.readlines()
            discardCurr = 0 if i == 0 else discardedFrames # Don't cut frames on first file
            assert(len(lines) > discardCurr + 1)
            outF.write(lines[0]) # Header line
            outF.writelines(lines[discardCurr + 1:]) # 1 accounts for header line







