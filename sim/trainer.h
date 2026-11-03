#pragma once

// trains LearnedBot's weights with evolution strategies: each generation tries pairs of mirrored
// random variations of the current weights, every variation plays its own match against ProtoBots
// in a child dynablaster_sim process, and the weights move towards the variations that scored
// better. Variations of one generation play on the same seed so they face the same luck.
//
//    dynablaster_sim --train [--generations n] [--population n] [--rounds n] [--duration s] [--bots n] [--learned n]
//                            [--sigma f] [--rate f] [--workers n] [--start file] [--out file] [--work dir] [--seed n]
//                            [--kill-weight f] [--relative yes]
//
// score per learned bot and round: 3 per kill (--kill-weight), -2 per death, 2 per win, 0.2 per extra;
// --relative yes subtracts the ProtoBots' score in the same match. Evaluations always print the plain
// score. --out gets the current weights after every generation plus a snapshot <out>.gNNN at every
// evaluation, the work directory the matches' files.
namespace Trainer
{
int run(int argc, char** argv);
}
