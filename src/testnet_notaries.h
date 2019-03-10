/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
 /* To use this testnet functionality we just need to pipe the output of getiguanajson RPC to this file.
 https://github.com/jl777/SuperNET/blob/master/iguana/m_notary_testnet#L11
example `gincoin-cli getiguanajson > testnet`
cp testnet ~/SuperNET/iguana/testnet
Also make sure the IP address's for a few or all of the testnet nodes are added to the m_notary_testnet file!
then to start iguana ./m_notary_testnet  
 */
 // you can change the port here to pretty much anything. Making hard forking changes to iguana, change the port. Other than that doesn't really matter.
 static const int32_t iguanaPort = 6666;
 static const int8_t BTCminsigs = 13;
 static const int8_t overrideMinSigs = 0;
 // I think these can really be anything... However its safer to have at least 1 or 2 testnet nodes IP here. (it must have 8 ips or iguana will crash)
 static const char *iguanaSeeds[8][1] =
 {
   {"149.28.187.139"},
   {"149.28.187.139"},
   {"149.28.187.139"},
   {"149.28.187.139"},
   {"149.28.187.139"},
   {"149.28.187.139"},
   {"149.28.187.139"},
   {"149.28.187.139"},
 };

// This needs to be a number higher than what the coin accepts as a "block from future"
// In komodo this is like 600s. This prevents a miner mining blocks with timestamps that could switch eras back and forth. 
 static const int STAKED_ERA_GAP = 777;
 
 // Disable and enable testnet mode with this var. To use komodo notary nodes, change to 0.
 static const int TESTNET = 1;

// To change pubkeys you need to add the new era's pubkeys to the following information. 
// eg to add era 3, change NUM_STAKED_ERAS to 3, and add the timestamp and number of pubkeys to:
// STAKED_NOTARIES_TIMESTAMP and num_notaries_STAKED. then add the pubkeys and notaries names to: 
// notaries_STAKED 3 dimensional array. 
 static const int NUM_STAKED_ERAS = 4;
 static const int STAKED_NOTARIES_TIMESTAMP[NUM_STAKED_ERAS] = { 1604533333, 1604633333 };
 static const int32_t num_notaries_STAKED[NUM_STAKED_ERAS] = { 10, 10 };

 // Era array of pubkeys.
 static const char *notaries_STAKED[NUM_STAKED_ERAS][64][2] =
 {
     {
         {"jorian", "0255fe006bac9fc6612fdcd5bc822075d232cfe960eb01c684ad82694186149dd5" },
         {"dukeleto", "02d93bbae25e55e5edf292b5e7f6455cc10738a4a7eaa5d5e7144ee08a52c3065c" },
         {"SHossain", "03d9b1c6676b99417be2dc8e22c47454334b2c7ca4adc1ae23616c7d52def4509d" },
         {"blackjok3r", "035fc678bf796ad52f69e1f5759be54ec671c559a22adf27eed067e0ddf1531574" },
         {"Decker", "027e52dc828517a8057936557fc29eb2da8fe7578c175bb3d194f5fda96dff5bf9" }, 
         {"mylo", "03f6b7fcaf0b8b8ec432d0de839a76598b78418dadd50c8e5594c0e557d914ec09" }, // RXN4hoZkhUkkrnef9nTUDw3E3vVALAD8Kx
         {"dummy1", "0344182c376f054e3755d712361672138660bda8005abb64067eb5aa98bdb40d10" }, // "REVPzjwEKgiU2H66pCWbAxsLheGysttJao" // Ut6pUbAbqjehNbSPEk26c9kTWbtyWozkum2DCMsixQCMCMC3ZMNV
         {"dummy2", "0215c8a2375fba2d64291c0497262344eea79c998f94f936ffeb27b3c39ccbb2fa" }, // "R9HLbsJHQ4K3hvTvCs6AfoURVtaAr2cyPr" // UsnFkE3m3XmKTjdxU7gL9NXPYVzKj76PDfDhy3F4sRAuar2oth3D
         {"dummy3", "02740a62802f34350c46e9ae7b80bbac15433de6266ac79bca2c3962ab1ab1d78b" }, // "RM5hkDri7hkCAa22kEk1YPoaVWAxgeRmpR" // UxRvxRZFMWsAwFoy83hBsgCU7cMC1LTY63v74AS68JwJ1am1y7GN
         {"dummy5", "03527c7ecd6a8c5db6d685a64e6e18c1edb49e2f057a434f56c3f1253a26e9c6a2" }, // RBw2jNU3dnGk86ZLqPMadJwRwg3NU8eC6s
     },
     {
         {"jorian", "0255fe006bac9fc6612fdcd5bc822075d232cfe960eb01c684ad82694186149dd5" },
         {"dukeleto", "02d93bbae25e55e5edf292b5e7f6455cc10738a4a7eaa5d5e7144ee08a52c3065c" },
         {"SHossain", "03d9b1c6676b99417be2dc8e22c47454334b2c7ca4adc1ae23616c7d52def4509d" },
         {"blackjok3r", "035fc678bf796ad52f69e1f5759be54ec671c559a22adf27eed067e0ddf1531574" },
         {"Decker", "027e52dc828517a8057936557fc29eb2da8fe7578c175bb3d194f5fda96dff5bf9" }, 
         {"mylo", "03f6b7fcaf0b8b8ec432d0de839a76598b78418dadd50c8e5594c0e557d914ec09" }, // RXN4hoZkhUkkrnef9nTUDw3E3vVALAD8Kx
         {"dummy1", "0344182c376f054e3755d712361672138660bda8005abb64067eb5aa98bdb40d10" }, // "REVPzjwEKgiU2H66pCWbAxsLheGysttJao" // Ut6pUbAbqjehNbSPEk26c9kTWbtyWozkum2DCMsixQCMCMC3ZMNV
         {"dummy2", "0215c8a2375fba2d64291c0497262344eea79c998f94f936ffeb27b3c39ccbb2fa" }, // "R9HLbsJHQ4K3hvTvCs6AfoURVtaAr2cyPr" // UsnFkE3m3XmKTjdxU7gL9NXPYVzKj76PDfDhy3F4sRAuar2oth3D
         {"dummy3", "02740a62802f34350c46e9ae7b80bbac15433de6266ac79bca2c3962ab1ab1d78b" }, // "RM5hkDri7hkCAa22kEk1YPoaVWAxgeRmpR" // UxRvxRZFMWsAwFoy83hBsgCU7cMC1LTY63v74AS68JwJ1am1y7GN
         {"dummy5", "03527c7ecd6a8c5db6d685a64e6e18c1edb49e2f057a434f56c3f1253a26e9c6a2" }, // RBw2jNU3dnGk86ZLqPMadJwRwg3NU8eC6s
     }
 };
